// Small functions from the miss461 batch (checked against the asm).
#include "types.h"

// Scoped lock record, as in props.cpp.
struct ScopedLock { u8 pad[4]; };

// 0x801B4090 (C++ linkage in small_801Bb.cpp)
void fn_801B4090(u8* p, u8* val);

extern "C" {
void    BufferFree(void* p);                                    // 0x801D4980
void*   EventPost(s32 id, void* sender);                       // 0x801F3918
s32     RaySceneTest(void* ray);                                // 0x80070458
void    ScopedLockCtor(ScopedLock* l);                          // 0x801F5384
void    ScopedLockDtor(ScopedLock* l, s32 flags);               // 0x801F53B0
void    operator_delete(void* p);                               // 0x801D426C
char*   strncpy(char* dst, const char* src, size_t n);

void    fn_8004BA50(u8* chr);                                   // 0x8004BA50
s32     fn_8005DB68(u8* a, s32 b);                              // 0x8005DB68 (signature guessed)
void    fn_80099D08(u8* self);                                  // 0x80099D08
void    fn_8009B320(u8* self);                                  // 0x8009B320
void    fn_8009A354(u8* p);                                     // 0x8009A354 (signature guessed)
u8*     fn_800E4BFC(u8* p);                                     // 0x800E4BFC
void    fn_800E4CB0(u8* arg0);                                  // 0x800E4CB0
u8*     fn_800E4D7C(u32 a0);                                    // 0x800E4D7C
void    fn_8017493C(void* p);                                   // 0x8017493C
void    fn_8018A948(u8* obj, f32 t);                            // 0x8018A948
u8*     fn_8018C48C(u8* base, u32 key);                         // 0x8018C48C
void    fn_8018C4D8(u8* p);                                     // 0x8018C4D8
s32     fn_8019BC80(u8* arg0, u32 arg1);                        // 0x8019BC80
s32     fn_800339A8(u8* arg0);                                  // 0x800339A8 (signature guessed)
u8*     fn_801AC184(u8* defs, s32 idx);                         // 0x801AC184
u8*     fn_801B3308(void);                                      // 0x801B3308
void    fn_801B3378(u8* p, u8* key);                            // 0x801B3378
void*   fn_801B373C(void* p);                                   // 0x801B373C
u8*     fn_801B3938(u8* a3);                                    // 0x801B3938
void    fn_801B4520(u8* p);                                     // 0x801B4520
void    fn_801B7A00(void* p);                                   // 0x801B7A00
void    fn_801BF398(u8* list, u8* item);                        // 0x801BF398
void    fn_801C849C(u8* mgr, u8* item);                         // 0x801C849C
void*   fn_801D40E0(s32 n);                                     // 0x801D40E0

extern const char lbl_80358538[];   // 0x80358538 (name string)
extern u8  lbl_8047C760[];          // 0x8047C760 (effect manager)
extern u8  lbl_8047C790[];          // 0x8047C790 (effect entry list)
extern u8  lbl_8047C7D8[];          // 0x8047C7D8
extern u8  lbl_803466B0[];          // 0x803466B0 (f32 table)
extern f32 lbl_803462C8[];          // 0x803462C8 (f32 table, element 0 is also a scalar)
extern f32 lbl_80346640[];          // 0x80346640 (f32 table)
extern f32 lbl_80346650[];          // 0x80346650 (f32 table)
extern f32 lbl_80346660[];          // 0x80346660 (f32 table)
extern u8  lbl_805FBB34;            // 0x805FBB34 (flag byte)
extern f32 lbl_805FCD88;            // 0x805FCD88 (float constant)
extern f32 lbl_805FCD90;            // 0x805FCD90 (float constant, 0.5)
extern f32 lbl_805FCD9C;            // 0x805FCD9C (float constant)
extern f32 lbl_805FDC50;            // 0x805FDC50 (float, 0.0)

// 0x8003FC00
// Allocates the 0x70-byte record (fn_801D40E0, then fn_801B373C), stores it at +0x84 and copies
// the name string into it (0x5F bytes, NUL at +0x5F).
void fn_8003FC00(u8* arg0) {
    u8* rec = (u8*)fn_801D40E0(0x70);
    if (rec != NULL) {
        rec = (u8*)fn_801B373C(rec);
    }
    *(void**)(arg0 + 0x84) = rec;
    strncpy((char*)rec, lbl_80358538, 0x5F);
    rec[0x5F] = 0;
}

// 0x8003FF30
// Once only (while the first byte of lbl_8047C7D8 is 0): registers the record at +0x84 and sets +0x98.
void fn_8003FF30(u8* arg0) {
    if (lbl_8047C7D8[0] == 0) {
        fn_801B4090(lbl_8047C7D8, *(u8**)(arg0 + 0x84));
        *(u8*)(arg0 + 0x98) = 1;
    }
}

// 0x8003FFD0
// Returns the word at +0x8 of the sub-record at +0xD4, or -1 when the record, that sub-record
// or the flag word at +0xCC is missing.
s32 fn_8003FFD0(u8* arg0) {
    u8* r = fn_801B3938(*(u8**)(arg0 + 0x84));
    if (r == NULL) {
        return -1;
    }
    u8* sub = *(u8**)(r + 0xD4);
    if (sub == NULL || *(s32*)(r + 0xCC) == 0) {
        return -1;
    }
    return *(s32*)(sub + 0x8);
}

// 0x8007CF54
// Deleting-destructor tail: frees p when del > 0, returns p.
u8* fn_8007CF54(u8* p, s32 del) {
    if (p != NULL && del > 0) {
        operator_delete(p);
    }
    return p;
}

// 0x801AC8A0
// Releases the entry for key arg1 in the table at arg0 (frees its +0x4 object, marks the slot -1),
// then posts event 0x7C to arg2 unless the flag 0x04000000 is set in arg2's +0x4 object.
void fn_801AC8A0(u8* arg0, s32 arg1, u8* arg2) {
    u8* slot = fn_8018C48C(arg0, (u32)arg1);
    if (slot != NULL) {
        u8* sub = *(u8**)(slot + 0x4);
        if (sub != NULL) {
            fn_8017493C(sub);
        }
        *(void**)(slot + 0x4) = NULL;
        *(s32*)(slot + 0x0) = -1;
        fn_8018C4D8(slot);
    }
    if (!(*(s32*)(*(u8**)(arg2 + 0x4) + 0x20) & 0x04000000)) {
        u8* ev = (u8*)EventPost(0x7C, arg2);
        if (ev != NULL) {
            *(s32*)(ev + 0x4) = -1;
            *(s32*)(ev + 0x8) = arg1;
        }
    }
}

// 0x8011D3A0
// State update: if vfunc 0x24 on arg0 returns 0, switches the active child (slots at +0x8, index at
// +0x18, enabled byte at +0x1C) to the index returned by vfunc 0x1C(arg1), with enter/exit calls.
void fn_8011D3A0(u8* arg0, s32 arg1) {
    if (((s32(*)(u8*))(*(void**)(*(u8**)arg0 + 0x24)))(arg0) != 0) {
        return;
    }
    s32 cur = *(s32*)(arg0 + 0x18);
    *(s32*)(arg0 + 0x4) = 0;
    if (cur != 4 && cur != -1 && *(u8*)(arg0 + 0x1C) != 0) {
        u8* child = *(u8**)(arg0 + 0x8 + cur * 4);
        ((void (*)(u8*))(*(void**)(*(u8**)child + 0x10)))(child);
    }
    s32 next = ((s32(*)(u8*, s32))(*(void**)(*(u8**)arg0 + 0x1C)))(arg0, arg1);
    *(s32*)(arg0 + 0x18) = next;
    if (next != -1 && *(u8*)(arg0 + 0x1C) != 0) {
        u8* child = *(u8**)(arg0 + 0x8 + next * 4);
        ((void (*)(u8*))(*(void**)(*(u8**)child + 0xC)))(child);
        *(void**)(arg0 + 0x4) = *(void**)(arg0 + 0x8 + *(s32*)(arg0 + 0x18) * 4);
    }
}

// 0x80126718
// Resets the state block: a few counters, the 0x100-byte table at +0x0 and the words at +0x118..+0x13C.
void fn_80126718(u8* arg0) {
    *(s32*)(arg0 + 0x140) = 4;
    *(s32*)(arg0 + 0x144) = 4;
    *(s32*)(arg0 + 0x148) = 0;
    *(s32*)(arg0 + 0x110) = 0;
    *(s32*)(arg0 + 0x114) = 0;
    *(s32*)(arg0 + 0x108) = 0;
    *(s32*)(arg0 + 0x100) = 0;
    *(s32*)(arg0 + 0x104) = 0x40;
    for (s32 off = 0; off < 0x100; off += 4) {
        *(s32*)(arg0 + off) = 0;
    }
    for (s32 off = 0x118; off <= 0x13C; off += 4) {
        *(s32*)(arg0 + off) = 0;
    }
}

// 0x802085C8
// Copies the record arg1 (0x95 bytes) into arg0 at +0xC, sets arg0+4 = 1, copies the word at
// arg1+0x98 and float at arg1+0x9C to +0xA4 / +0xA8, and sets lbl_805FBB34. Returns 1.
s32 fn_802085C8(u8* arg0, u8* arg1) {
    lbl_805FBB34 = 1;
    *(s32*)(arg0 + 0x4) = 1;
    for (s32 i = 0; i < 0x95; i++) {
        arg0[0xC + i] = arg1[i];
    }
    *(s32*)(arg0 + 0xA4) = *(s32*)(arg1 + 0x98);
    *(f32*)(arg0 + 0xA8) = *(f32*)(arg1 + 0x9C);
    return 1;
}

// 0x8004CC3C
// Per-frame update: fn_8004BA50(arg0); vfunc 0x18(arg1) on each of the 5 objects at +0xC50 under a
// scoped lock; for the 2 objects at +0xC64 detach, vfunc 0x8(1) and clear the slot; then fn_8005DB68.
void fn_8004CC3C(u8* arg0, s32 arg1) {
    ScopedLock lock;
    fn_8004BA50(arg0);
    for (s32 i = 0; i < 5; i++) {
        u8* obj = *(u8**)(arg0 + 0xC50 + i * 4);
        if (obj != NULL) {
            ScopedLockCtor(&lock);
            ((void (*)(u8*, s32))(*(void**)(*(u8**)obj + 0x18)))(obj, arg1);
            ScopedLockDtor(&lock, -1);
        }
    }
    for (s32 i = 0; i < 2; i++) {
        u8* obj = *(u8**)(arg0 + 0xC64 + i * 4);
        if (obj != NULL) {
            fn_801B3378(fn_801B3308(), obj);
            u8* obj2 = *(u8**)(arg0 + 0xC64 + i * 4);
            if (obj2 != NULL) {
                ((void (*)(u8*, s32))(*(void**)(*(u8**)obj2 + 0x8)))(obj2, 1);
            }
            *(void**)(arg0 + 0xC64 + i * 4) = NULL;
        }
    }
    fn_8005DB68(arg0, arg1);
}

// 0x80084C18
// Blend value at +0x26C: a table lookup when the byte at +0x285 is set, otherwise the smaller of
// the table value and (lbl_80346640 - lbl_80346650 * int field) chosen by RaySceneTest indices.
void fn_80084C18(u8* arg0) {
    f32 v;
    if (arg0[0x285] != 0) {
        v = ((f32*)lbl_803466B0)[RaySceneTest(arg0)];
    } else {
        s32 i0 = RaySceneTest(arg0);
        f32 f30 = lbl_80346640[i0];
        s32 i1 = RaySceneTest((void*)(intptr_t)i0);
        f32 f31 = lbl_80346650[i1];
        s32 i2 = RaySceneTest((void*)(intptr_t)i1);
        v = lbl_80346660[i2];
        f32 t = f30 - f31 * (f32)*(s32*)(*(u8**)(arg0 + 0x264) + 0x334);
        if (t <= v) {
            v = t;
        }
    }
    *(f32*)(arg0 + 0x26C) = v;
}

// 0x8009A548
// Actor setup: when lbl_803462C8[0] > lbl_805FDC50 stores lbl_803462C8[ray index] at +0x8C; runs
// fn_80099D08 and fn_8009B320, then sets flags on the child object from fn_800E4D7C.
void fn_8009A548(u8* arg0) {
    fn_8009A354(arg0);
    void* ray = NULL;  // RaySceneTest ignores its argument
    if (lbl_803462C8[0] > lbl_805FDC50) {
        *(f32*)(arg0 + 0x8C) = lbl_803462C8[RaySceneTest(ray)];
    }
    fn_80099D08(arg0);
    fn_8009B320(arg0 + 0x180);
    // r3 for this call is not set by the asm (fn_8009B320 returns void), so it is passed as 0.
    u8* p = fn_800E4D7C(0);
    u8* c = fn_800E4BFC(p);
    u8* child = fn_801AC184(*(u8**)(*(u8**)(c + 0x9C) + 0x48), *(s32*)(p + 0x5C));
    *(s32*)(child + 0xC) |= 0x10000;
    *(u8*)(p + 0x54) = 1;
    u8* o = fn_800E4BFC(p);
    u8* ret = ((u8*(*)(u8*))(*(void**)(*(u8**)o + 0x10)))(o);
    fn_800E4CB0(p);
    fn_8018A948(ret, lbl_805FDC50);
    u8* q = fn_800E4BFC(p);
    *(s32*)(q + 0x15C) |= 1;
}

// 0x80033870
// Per-frame update: clears +0x4/+0x8 and sets +0x24; if the object at +0x18 exists, sets its
// fields, calls vfunc 0x2C of its sub-object, then copies the scaled extents to +0x74..+0x7C.
void fn_80033870(u8* arg0) {
    *(s32*)(arg0 + 0x8) = 0;
    *(f32*)(arg0 + 0x24) = lbl_805FCD88;
    *(s32*)(arg0 + 0x4) = 0;
    u8* obj = *(u8**)(arg0 + 0x18);
    if (obj == NULL) {
        return;
    }
    *(f32*)(obj + 0x68) = lbl_805FCD9C;
    *(f32*)(obj + 0x6C) = lbl_805FCD9C;
    *(f32*)(obj + 0x70) = lbl_805FCD88;
    // Call through the function pointer at +0x2C of the object stored at obj+0x138 (f1 is left live).
    ((void (*)(u8*, s32))(*(void**)(*(u8**)(obj + 0x138) + 0x2C)))(obj, 0x38);
    obj = *(u8**)(arg0 + 0x18);
    if (*(s32*)(obj + 0x10) != 0) {
        fn_8019BC80(obj, 1);
    }
    s32 w = fn_800339A8(arg0);
    *(s32*)(arg0 + 0xC) = w;
    f32 c = lbl_805FCD90;
    f32 fx = *(f32*)(arg0 + 0x3C) * c;
    f32 fy = *(f32*)(arg0 + 0x40) * c;
    f32 fz = *(f32*)(arg0 + 0x44) * c;
    u8* o = *(u8**)(arg0 + 0x18);
    *(f32*)(o + 0x74) = fx;
    *(f32*)(o + 0x78) = fy;
    *(f32*)(o + 0x7C) = fz;
}

// 0x8006D68C
// Teardown: removes the two entries at +0x17C from lbl_8047C790, posts event 0xC unless the flag
// 0x04000000 is set on the object at +0x4, releases the entry at +0x178 under a lock, frees the
// buffer at +0x204, then calls fn_801B4520.
void fn_8006D68C(u8* arg0) {
    ScopedLock lock;
    for (s32 i = 0; i < 2; i++) {
        u8* item = *(u8**)(arg0 + 0x17C + i * 4);
        if (item != NULL) {
            fn_801BF398(lbl_8047C790, item);
            *(void**)(arg0 + 0x17C + i * 4) = NULL;
        }
    }
    if (!(*(s32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000)) {
        EventPost(0xC, arg0);
    }
    if (*(void**)(arg0 + 0x178) != NULL) {
        ScopedLockCtor(&lock);
        fn_801C849C(lbl_8047C760, *(u8**)(arg0 + 0x178));
        *(void**)(arg0 + 0x178) = NULL;
        ScopedLockDtor(&lock, -1);
    }
    *(s32*)(arg0 + 0x1AC) = 0;
    if (*(void**)(arg0 + 0x204) != NULL) {
        fn_801B7A00(*(void**)(arg0 + 0x204));
        BufferFree(*(void**)(arg0 + 0x204));
        *(void**)(arg0 + 0x204) = NULL;
    }
    fn_801B4520(arg0);
}

}  // extern "C"
