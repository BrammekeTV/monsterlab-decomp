// Small functions in 0x8006xxxx (entity base helpers).
#include "types.h"
#include <math.h>

typedef void  (*VirtFnSelf)(u8* self);
typedef void  (*VirtFnSetFloat)(u8* self, f32 t);
typedef void* (*VirtFnSelfPtr)(u8* self);

// Sets bits in the 32-bit word at +0x1C of the child object at +4, and its byte at +0x34 to state.
// Does nothing when the child is missing.
static void SetChildStateBits(u8* p, u32 bits, u8 state) {
    u8* child = *(u8**)(p + 0x4);
    if (child == NULL) return;
    *(u32*)(child + 0x1C) |= bits;
    *(u8*)(child + 0x34) = state;
}

extern "C" {
void     operator_delete(void* p);                                              // 0x801D426C
void     SpawnEffectAt(void* fxMgr, void* effect, const f32 posAndScale[4], int a); // 0x801C80E0
void     fn_80061A98(u8* p, s32* a, s32 v);                                     // 0x80061A98
u8*     fn_801B44AC(u8* p, s32 del);                                           // 0x801B44AC (base dtor)
void     fn_801B4334(u8* p);                                                    // 0x801B4334 (base ctor)
void     fn_801B4944(u8* p);                                                    // 0x801B4944
void     fn_80122B00(void* a, u8* b);                                           // 0x80122B00
u8* fn_8006F61C(u8* arg0);                                                    // 0x8006F61C
void     fn_8006F67C(u8* p, f32 t, s32 flag);                                   // 0x8006F67C (leaf version takes only p)
u8*     fn_8005CA88(u8* p);                                                    // 0x8005CA88
void     fn_8005CB40(u8* p, f32 t, s32 flag);                                   // 0x8005CB40
void     fn_800655D8(u8* p);                                                    // 0x800655D8
void     fn_8032F1E8(u8* obj, void (*cb)(void), s32 a, s32 b, s32 c);    // 0x8032F1E8
void     fn_80062C7C(void);                                                     // 0x80062C7C
void     fn_801B6ACC(u8* obj, s32 value);                                       // 0x801B6ACC
void     fn_801DAA40(u8* a, u8* b);                                              // 0x801DAA40
void     fn_801DAA74(u8* a, u8* b);                                              // 0x801DAA74
void     operator_delete(void* p);                                               // 0x801D426C
void     fn_801B5D94(u8* obj, u8* event);                                       // 0x801B5D94
void     fn_801B1668(void* p);                                                   // 0x801B1668
void     fn_801B4520(u8* p);                                                     // 0x801B4520
void     BufferFree(void* p);                                                    // 0x801D4980
void     fn_801ACCEC(void);                                                      // 0x801ACCEC
void     TriggerDoAction(void* action, void* a1, void* a2);                     // 0x801E0054
void     fn_80121C90(s32 action, void* obj, void* other, const f32* pos);       // 0x80121C90
BOOL     PropIncrementState(void* prop);                                        // 0x80069FDC
void     fn_80068B50(u8* obj, s32 mode);                                        // 0x80068B50
u8*       fn_8005CAE8(u8* obj, s32 mode);                                       // 0x8005CAE8
void     fn_80184488(void* owner, u8* attached);                                // 0x80184488
void     fn_801D9B34(void* a, f32 t);                                           // 0x801D9B34
void*    fn_80172F70(s32 a, const void* b);                                     // 0x80172F70
void     fn_80172E84(u8* obj);                                                  // 0x80172E84
u8*      fn_801842C8(u8* a);                                                    // 0x801842C8
void     fn_80184590(u8* a, u8* b);                                             // 0x80184590
u8*      fn_80062CE0(u8* p, s32 del);                                           // 0x80062CE0 (deleting dtor)

extern f32 lbl_805FD3F0;           // 0x805FD3F0 (float constant)
extern f32 lbl_805FD3FC;           // 0x805FD3FC (float constant)
extern f32 lbl_805FD468;           // 0x805FD468 (float constant, fourth element of the effect position)
extern f32 lbl_805FD48C;           // 0x805FD48C (lower bound of the range check)
extern f32 lbl_805FD490;           // 0x805FD490 (upper bound of the range check)
extern f32 lbl_803C5680[3];        // 0x803C5680
extern u8  lbl_803C54F0[];         // 0x803C54F0
extern u8  lbl_80359BC8[];         // 0x80359BC8 (vtable)
extern u8  lbl_80359C88[];         // 0x80359C88 (lookup key)
extern u8  lbl_80359C94[];         // 0x80359C94 (vtable)
extern u8  lbl_80359ED8[];         // 0x80359ED8 (vtable)
extern u8  gWorldState[];          // 0x803E6D00

// 0x800609E0
// Writes (key, flag) into the first slot of the 8-entry table at +0xBE0 (stride 8) that is empty or
// already holds key. Does nothing when the table is full.
void fn_800609E0(u8* p, s32 key, s32 flag) {
    for (s32 i = 0; i < 8; ++i) {
        u8* slot = p + 0xBE0 + i * 8;
        s32 cur = *(s32*)slot;
        if (cur == 0 || cur == key) {
            *(s32*)slot = key;
            *(u8*)(slot + 4) = (u8)flag;
            return;
        }
    }
}

// 0x80060FA8
// Returns 1 if the distance between this object's position (+4) and the position of the
// object at +0xA14 is less than radius, 0 otherwise (also 0 when there is no object at +0xA14).
s32 fn_80060FA8(u8* p, f32 radius) {
    u8* target = *(u8**)(p + 0xA14);
    if (target == NULL) return 0;
    const f32* a = *(const f32**)(target + 0x4);
    const f32* b = *(const f32**)(p + 0x4);
    f32 dx = a[0] - b[0];
    f32 dy = a[1] - b[1];
    f32 dz = a[2] - b[2];
    f32 r2 = radius * radius;
    f32 d2 = dx * dx + dy * dy + dz * dz;
    return d2 < r2;
}

// 0x80061A64
// Stores *b at +0xBD0 and passes a copy of *a together with *b to fn_80061A98.
void fn_80061A64(u8* p, s32* a, s32* b) {
    *(s32*)(p + 0xBD0) = *b;
    s32 copy = *a;
    fn_80061A98(p, &copy, *b);
}

// 0x80062398
// Calls virtual slot 0x4C on the object at +0xA14, if there is one. Always returns 0.
s32 fn_80062398(u8* p) {
    u8* obj = *(u8**)(p + 0xA14);
    if (obj != NULL) {
        (*(VirtFnSelf*)(*(u8**)obj + 0x4C))(obj);
    }
    return 0;
}

// 0x80062C40
// Sets three float constants in lbl_803C5680 and tail-calls fn_8032F1E8 with fn_80062C7C.
void fn_80062C40(void) {
    lbl_803C5680[0] = lbl_805FD3F0;
    lbl_803C5680[1] = lbl_805FD3FC;
    lbl_803C5680[2] = lbl_805FD3F0;
    fn_8032F1E8(lbl_803C54F0, fn_80062C7C, 0, 0x18, 0x10);
}

// 0x80062C80
// Base constructor: calls fn_801B4334, sets the vtable and marks the child at +4 with bits 0x2, state 1.
u8* fn_80062C80(u8* p) {
    fn_801B4334(p);
    *(void**)p = lbl_80359BC8;
    SetChildStateBits(p, 0x2, 1);
    return p;
}

// 0x80062CE0
// Deleting destructor: calls the base dtor fn_801B44AC(p, 0) if p is non-null, then frees p when del > 0.
// Returns p.
u8* fn_80062CE0(u8* p, s32 del) {
    if (p != NULL) {
        fn_801B44AC(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80062F58
// Calls the object's virtual slot 0xB8; if it returns a non-null object, calls fn_80122B00 with it and p.
void fn_80062F58(u8* p) {
    fn_801B4944(p);
    void* result = (*(VirtFnSelfPtr*)(*(u8**)p + 0xB8))(p);
    if (result != NULL) {
        fn_80122B00(result, p);
    }
}

// 0x8006307C
// Base constructor: calls fn_8006F61C, sets the vtable and marks the child at +4 with bits 0x400, state 0xA.
u8* fn_8006307C(u8* p) {
    fn_8006F61C(p);
    *(void**)p = lbl_80359C94;
    SetChildStateBits(p, 0x400, 0xA);
    return p;
}

// 0x800630E0
// Deleting destructor: fn_80062CE0(p, 0) if p is non-null, then frees p when del > 0. Returns p.
u8* fn_800630E0(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80063318
// Per-frame update of the object at +4: when bit 0 of its word at +0x20 is set, calls fn_801D9B34 with the
// object at +0x90 and the world's float at gWorldState+0xC, then fn_8006F67C(p, t, 0).
void fn_80063318(u8* p, f32 t) {
    u8* child = *(u8**)(p + 0x4);
    if (*(u32*)(child + 0x20) & 1) {
        fn_801D9B34(*(void**)(p + 0x90), *(f32*)(gWorldState + 0xC));
        fn_8006F67C(p, t, 0);
    }
}

// 0x80063388
// Looks up the object for key lbl_80359C88; when p is NULL, sets bit 0x8 in its s16 word at +0x12.
void fn_80063388(u8* p) {
    u8* r = (u8*)fn_80172F70(1, lbl_80359C88);
    if (p == NULL) {
        *(s16*)(r + 0x12) = (s16)(*(s16*)(r + 0x12) | 0x8);
    }
}

// 0x800634D0
// If fn_801842C8 on the object at +0xC is non-null, passes the second such result and p to fn_80184590.
void fn_800634D0(u8* p) {
    if (p == NULL) return;
    if (fn_801842C8(*(u8**)(p + 0xC)) == NULL) return;
    u8* r = fn_801842C8(*(u8**)(p + 0xC));
    fn_80184590(r, p);
}

// 0x80063530
// Copies four floats from v into the object at +0x44..+0x50 and calls fn_80172E84. The object is p, or
// the lookup result for lbl_80359C88 when p is NULL.
void fn_80063530(u8* a, const f32* v) {
    u8* obj = a;
    if (obj == NULL) obj = (u8*)fn_80172F70(1, lbl_80359C88);
    if (obj == NULL) return;
    *(f32*)(obj + 0x44) = v[0];
    *(f32*)(obj + 0x48) = v[1];
    *(f32*)(obj + 0x4C) = v[2];
    *(f32*)(obj + 0x50) = v[3];
    fn_80172E84(obj);
}

// 0x80063624
// Deleting destructor: fn_80062CE0(p, 0) if p is non-null, then frees p when del > 0. Returns p.
u8* fn_80063624(u8* p, s32 del) {
    if (p != NULL) {
        fn_80062CE0(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800636BC
// Base constructor: calls fn_8005CA88, sets the vtable and marks the child at +4 with bits 0x80, state 7.
u8* fn_800636BC(u8* p) {
    fn_8005CA88(p);
    *(void**)p = lbl_80359ED8;
    SetChildStateBits(p, 0x80, 7);
    return p;
}

// 0x80064568
// Spawns an effect at the position v (x, y, z and the constant lbl_805FD468 as the fourth element).
// fxMgr, effect and a are passed through to SpawnEffectAt unchanged.
void fn_80064568(void* fxMgr, void* effect, const f32* v, s32 a) {
    f32 pos[4] = { v[0], v[1], v[2], lbl_805FD468 };
    SpawnEffectAt(fxMgr, effect, pos, a);
}

// 0x80065CB0
// Returns 1 if all three components of the vector lie strictly between lbl_805FD48C and lbl_805FD490.
s32 fn_80065CB0(const f32* v) {
    for (s32 i = 0; i < 3; ++i) {
        if (v[i] <= lbl_805FD48C || v[i] >= lbl_805FD490) return 0;
    }
    return 1;
}

// 0x80065EC8
// Updates the object's float timer t: if bit 0x04000000 of its child's word at +0x20 is set, does nothing.
// If bit 0x80000000 of +0x244 is set, calls virtual slot 0x58 with t. Otherwise calls fn_800655D8 and
// fn_8005CB40(p, t, 0).
void fn_80065EC8(u8* p, f32 t) {
    u8* child = *(u8**)(p + 0x4);
    if (*(u32*)(child + 0x20) & 0x04000000) return;
    if (*(u32*)(p + 0x244) & 0x80000000) {
        (*(VirtFnSetFloat*)(*(u8**)p + 0x58))(p, t);
        return;
    }
    fn_800655D8(p);
    fn_8005CB40(p, t, 0);
}

// 0x800677A0
// Returns 1 if the x component of the vector lies strictly between lbl_805FD48C and lbl_805FD490.
s32 fn_800677A0(const f32* v) {
    f32 x = v[0];
    if (x <= lbl_805FD48C || x >= lbl_805FD490) return 0;
    return 1;
}

// 0x80062FA8
// Clears the currently attached effect data when the object's attachment key changes,
// then records the new key through the shared effect manager.
void fn_80062FA8(u8* obj, u32 key) {
    u32 oldKey = *(u32*)(obj + 0x88);
    u8* attached = (u8*)(*(VirtFnSelfPtr*)(*(u8**)obj + 0xB8))(obj);
    if (attached != NULL && oldKey != key) {
        for (s32 off = 0; off <= 0x3C; off += 4) *(u32*)(attached + off) = 0;
    }
    fn_801B6ACC(obj, (s32)key);
}

// 0x80063138
// Marks an entity as active and, when its zone is resident, refreshes the world's
// attachment bookkeeping before enabling the entity's scene node.
void fn_80063138(u8* obj) {
    u8* node = *(u8**)(obj + 4);
    u8* attach = *(u8**)(obj + 0x90);
    if (attach != NULL && !(*(u32*)(node + 0x20) & 0x40)) {
        u16 zone = *(u16*)(node + 0x30);
        u8* loaded = zone == 0xFFFF ? *(u8**)(*(u8**)(gWorldState + 0x1B0))
                                    : *(u8**)(*(u8**)(gWorldState + 0x1B0) + zone * 4);
        if (loaded != NULL) fn_801DAA40(loaded, attach);
    }
    node = *(u8**)(obj + 4);
    *(u32*)(node + 0x20) &= ~0x20500u;
    if ((*(VirtFnSelfPtr*)(*(u8**)obj + 0xC))(obj) != NULL) {
        u8* sub = (u8*)(*(VirtFnSelfPtr*)(*(u8**)obj + 0xC))(obj);
        *(u32*)(sub + 8) |= 2;
    }
    node = *(u8**)(obj + 4);
    *(u32*)(node + 0x20) |= 3;
}

// 0x8006322C
// Disables an entity's scene node, clears its attachment bit, and notifies the
// world when a resident zone is being released.
void fn_8006322C(u8* obj) {
    u8* node = *(u8**)(obj + 4);
    if (node == NULL) return;
    if ((*(VirtFnSelfPtr*)(*(u8**)obj + 0xC))(obj) != NULL) {
        u8* sub = (u8*)(*(VirtFnSelfPtr*)(*(u8**)obj + 0xC))(obj);
        *(u32*)(sub + 8) &= ~2u;
    }
    node = *(u8**)(obj + 4);
    *(u32*)(node + 0x20) &= ~0x20503u;
    u8* attach = *(u8**)(obj + 0x90);
    node = *(u8**)(obj + 4);
    if (attach != NULL && !(*(u32*)(node + 0x20) & 0x40)) {
        u16 zone = *(u16*)(node + 0x30);
        u8* loaded = zone == 0xFFFF ? *(u8**)(*(u8**)(gWorldState + 0x1B0))
                                    : *(u8**)(*(u8**)(gWorldState + 0x1B0) + zone * 4);
        if (loaded != NULL) fn_801DAA74(loaded, attach);
    }
}

// 0x800633D4
// Re-attaches the named world attachment to the current zone, preserving its
// position and resetting the fourth component used by the attachment system.
u8* fn_800633D4(u8* attachment) {
    if (attachment == NULL) attachment = (u8*)fn_80172F70(1, lbl_80359C88);
    if (attachment == NULL) return attachment;
    u8* world = *(u8**)(gWorldState + 8);
    if (world == NULL) return attachment;
    u8* current = *(u8**)(world + 4);
    if (current == NULL) return attachment;
    u16 zone = *(u16*)(current + 0x30);
    if (zone == 0 || zone > *(s32*)(gWorldState + 4)) return attachment;
    u8* zoneObj = *(u8**)(*(u8**)(gWorldState + 0x1B0) + zone * 4);
    s16 kind = *(s16*)(attachment + 0x10);
    if ((kind != 1 && kind != 2) || *(u8**)(zoneObj + 0x4C) == NULL) return attachment;
    f32 pos[4] = { *(f32*)(attachment + 0x44), *(f32*)(attachment + 0x48), *(f32*)(attachment + 0x4C), 0.0f };
    *(f32*)(attachment + 0x50) = 0.0f;
    fn_80172E84(attachment);
    fn_80184488(*(void**)(zoneObj + 0x4C), attachment);
    (void)pos;
    return attachment;
}

// 0x8006359C
// Configures attachment interpolation for a positive travel time.
void fn_8006359C(u8* attachment, f32 duration) {
    if (attachment == NULL) attachment = (u8*)fn_80172F70(1, lbl_80359C88);
    if (attachment == NULL || duration <= 0.0f) return;
    *(f32*)(attachment + 0x54) = 1.0f;
    *(f32*)(attachment + 0x58) = 0.0f;
    *(f32*)(attachment + 0x5C) = 5.0f / duration;
    fn_80172E84(attachment);
}

// 0x8006371C
// Releases the entity's attached resources before invoking its base destructor.
u8* fn_8006371C(u8* obj, s32 del) {
    if (obj != NULL) {
        *(void**)obj = lbl_80359ED8;
        fn_80068B50(obj, 1);
        void* buffer = *(void**)(obj + 0x22C);
        if (buffer != NULL) {
            fn_801ACCEC();
            BufferFree(buffer);
            *(void**)(obj + 0x22C) = NULL;
        }
        fn_8005CAE8(obj, 0);
        if (del > 0) operator_delete(obj);
    }
    return obj;
}

// 0x8006680C
// Applies a vector to every child entry, or resets all child transforms when the
// incoming vector is effectively zero.
void fn_8006680C(u8* list, const f32* vec) {
    f32 x = vec[0], y = vec[1], z = vec[2], w = 0.0f;
    BOOL zero = x <= lbl_805FD48C && x >= lbl_805FD490 && y <= lbl_805FD48C && y >= lbl_805FD490 &&
                z <= lbl_805FD48C && z >= lbl_805FD490 && w <= lbl_805FD48C && w >= lbl_805FD490;
    for (s32 i = 0; i < *(s32*)(list + 0xC); ++i) {
        u8* child = *(u8**)(list + 0x14 + i * 4);
        if (child == NULL) continue;
        *(f32*)(child + 0x30) = zero ? 0.0f : x;
        *(f32*)(child + 0x34) = zero ? 0.0f : y;
        *(f32*)(child + 0x38) = zero ? 1.0f : z;
        *(f32*)(child + 0x3C) = zero ? 0.0f : w;
    }
}

// 0x800676AC
// Returns 1 if the difference a - b lies strictly between lbl_805FD490 and lbl_805FD48C (+-0.0001).
s32 fn_800676AC(f32 a, f32 b) {
    f32 d = a - b;
    return d > lbl_805FD490 && d < lbl_805FD48C;
}

// 0x800676E0
// Normalizes a vector and returns its original length through outLength.
void fn_800676E0(f32* out, const f32* in, f32* outLength) {
    f32 lenSq = in[0] * in[0] + in[1] * in[1] + in[2] * in[2];
    if (lenSq == 0.0f) {
        out[0] = out[1] = out[2] = 0.0f;
        *outLength = 0.0f;
        return;
    }
    f32 len = sqrtf(lenSq);
    *outLength = len;
    f32 inv = 1.0f / len;
    out[0] = in[0] * inv;
    out[1] = in[1] * inv;
    out[2] = in[2] * inv;
}

// 0x80067AEC
// Delivers a collision action to the entity's configured action and to any
// trigger actions carried by the other object.
void fn_80067AEC(u8* obj, u8* other, const f32* pos) {
    u8* def = *(u8**)(obj + 0x228);
    if (def != NULL && *(s32*)(def + 0x14C) != 0) {
        f32 p[3] = { pos[0], pos[1], pos[2] };
        fn_80121C90(*(s32*)(def + 0x14C), obj, other, p);
    }
    if (other != NULL) {
        u8* action = *(u8**)(other + 0x9C);
        if (action != NULL && ((*(u32*)(action + 0x74) >> 27) & 0xF) != 0)
            TriggerDoAction(action + 0x74, other, obj);
        if (((*(u32*)(other + 0x44) >> 27) & 0xF) != 0)
            TriggerDoAction(other + 0x44, other, obj);
    }
}
}
