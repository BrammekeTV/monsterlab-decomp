// Small functions from the miss462 batch: entity/prop setup and teardown, event sends,
// segment/vector helpers and a few state checks.
#include <math.h>
#include <stdint.h>
#include "types.h"

extern "C++" void* gEventMgr;              // 0x805FB6C8

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
void  EntitySyncTransform(void* entity, int a);                 // 0x801B4F4C
void  MemFreeSmall(void* p);                                    // 0x801D4340
void  TextureRelease(void* tex);                                // 0x80184028
void  operator_delete(void* p);                                 // 0x801D426C
u32   OSGetBusClock(void);                                      // *(u32*)0x800000F8
u32   fn_800C2618(u8* a3, s32 a4);                              // 0x800C2618
u8*   fn_800C2028(u8* a0);                                       // 0x800C2028
void  fn_800C2760(u8* p, u8* owner);                            // 0x800C2760
u8*   fn_8006FBDC(void);                                        // 0x8006FBDC
u8*   fn_800E4D7C(u8* a0);                                      // 0x800E4D7C
void  fn_80070460(u8* p);                                       // 0x80070460
u8*   fn_80062CE0(u8* p, s32 del);                              // 0x80062CE0 (base dtor)
u8*   fn_801B3308(void);                                        // 0x801B3308
void  fn_801B3378(u8* p, u8* key);                              // 0x801B3378
void  fn_801EC080(u8* a, u8* b);                                // 0x801EC080
u8*   fn_801F6238(u8* p, s32 del);                              // 0x801F6238
void  fn_8002A814(u8* p);                                       // 0x8002A814
void  fn_8002C540(void* out, u8* src, f32 scale);               // 0x8002C540
void  fn_80029F30(u8* p, void* vec);                            // 0x80029F30
u8*   fn_800216B8(u8* p, s32 del);                              // 0x800216B8 (deleting dtor)
void  fn_802E93CC(void);                                        // 0x802E93CC
u8*   fn_8010B5A0(u8* p, u8* src);                                // 0x8010B5A0
void  fn_8011B060(u8* a, u8* b);                                // 0x8011B060
void  fn_8023A5B0(u8* p);                                       // 0x8023A5B0
void  fn_8023A68C(u8* p);                                       // 0x8023A68C
void  fn_8023D3C8(s32 hi, u32 lo);                              // 0x8023D3C8
f32   fn_8033F200(f32 a, f32 b);                                // 0x8033F200

extern u8  lbl_8035DA50[];       // 0x8035DA50 (vtable)
extern u8  lbl_80357408[];       // 0x80357408 (vtable)
extern u8  lbl_80556AB0[];       // 0x80556AB0 (connection lock object)
extern u32 lbl_805F8C78[];       // 0x805F8C78 (two tags: [0] at 805F8C78, [1] at 805F8C7C)
extern u32 lbl_805F8C80[];       // 0x805F8C80 (two tags: [0] at 805F8C80, [1] at 805F8C84)
extern void* lbl_805FAFB8;       // 0x805FAFB8
extern u8  lbl_805FB2A4[];       // 0x805FB2A4 (byte flag)
extern s32 lbl_805FB380;         // 0x805FB380
extern s32 lbl_805FBB48;         // 0x805FBB48 (connection nesting counter)
extern s32 lbl_805FBB64;         // 0x805FBB64
extern s32 lbl_805FBB68;         // 0x805FBB68
extern s32 lbl_805FBB6C;         // 0x805FBB6C
extern s32 lbl_805FBB70;         // 0x805FBB70
extern u8  lbl_805FBBE0[];       // 0x805FBBE0 (save block pointer)
extern f32 lbl_805FCB18;         // 0x805FCB18 (float threshold)
extern f32 lbl_805FCB24;         // 0x805FCB24 (float offset)
extern f32 lbl_805FCB80;         // 0x805FCB80 (float constant, zero-length value)
extern f32 lbl_805FDCCC;         // 0x805FDCCC (float argument)
extern f64 lbl_805FDCD0;         // 0x805FDCD0 (double argument)
extern f64 lbl_805FDCD8;         // 0x805FDCD8 (double argument, 2*pi)
extern f32 lbl_805FDFC0;         // 0x805FDFC0 (float default)
extern f32 lbl_805FDFC4;         // 0x805FDFC4 (float default, 1.0)
extern f32 lbl_805FFAB8;         // 0x805FFAB8 (float threshold)
}

extern "C" {

// 0x800C4364
// Looks up the entry for index +0x6C in the table at +0x78 (range +0x70..+0x74), stores the
// entry address at +0x7C and passes the entry's +0x2A0 value to fn_8010B5A0 (returned).
s32 fn_800C4364(u8* arg0, u8* arg1) {
    fn_800C2760(arg0, arg1);
    s32 idx = *(s32*)(arg0 + 0x6C);
    u8* cur = 0;
    if (idx >= 0 && *(s32*)(arg0 + 0x70) >= 0) {
        s32 size = *(s32*)(arg0 + 0x74);
        bool ok = false;
        if (idx >= 0 && idx < size) {
            s32 hi = *(s32*)(arg0 + 0x70);
            if (hi < idx) hi += size;
            ok = (idx <= hi);
        }
        u8* entry = 0;
        if (ok) entry = *(u8**)(arg0 + 0x78) + idx * 4;
        *(u8**)(arg0 + 0x7C) = entry;
        if (entry != 0) cur = *(u8**)entry;
    }
    if (cur == 0) {
        // The asm reads the word at address 0x2A0 here (NULL object); the PC build passes 0.
        return (s32)(intptr_t)fn_8010B5A0(arg0 + 0xD8, NULL); // lint:ok
    }
    return (s32)(intptr_t)fn_8010B5A0(arg0 + 0xD8, *(u8**)(cur + 0x2A0)); // lint:ok
}

// 0x8002B3CC
// For object kinds 3 and 4: refreshes the object, computes a scaled offset from its fields
// and, when the value at +0xB8 is at or above the threshold, calls virtual slot 0x1C.
void fn_8002B3CC(u8* arg0, f32 farg0) {
    s32 kind = *(s32*)(arg0 + 0x18);
    if (kind != 3 && kind != 4) return;
    fn_8002A814(arg0);
    f32 vec[3];
    f32 scale = farg0 * (*(f32*)(arg0 + 0x4) * (f32)*(s32*)(arg0 + 0x8));
    fn_8002C540(vec, arg0 + 0xB8, scale);
    fn_80029F30(arg0, vec);
    typedef void (*FnV)(void*);
    if ((*(f32*)(arg0 + 0xB8) - lbl_805FCB24) >= lbl_805FCB18) {
        (*(FnV*)(*(u8**)arg0 + 0x1C))(arg0);
    }
}

// 0x8006D9A0
// Counts active sub-entries: five 8-byte slots when the tag at +0x9C -> +8 is 0x6DB80004 (the
// virtual at 0x4C reports 0), otherwise five fixed pointer/flag fields.
s32 fn_8006D9A0(u8* arg0) {
    s32 count = 0;
    u32 tag = *(u32*)(*(u8**)(arg0 + 0x9C) + 0x8);
    if (tag - 0x6DB80000u == 4u) {
        typedef s32 (*FnS)(void*);
        u8* cur = arg0;
        for (s32 i = 0; i < 5; i++) {
            u8* obj = *(u8**)(cur + 0x1D4);
            if (obj != 0 && (*(FnS*)(*(u8**)obj + 0x4C))(obj) == 0) {
                count += 1;
            }
            cur += 8;
        }
    } else {
        if (*(s32*)(arg0 + 0x1D4) != 0) count = 1;
        if (*(s32*)(arg0 + 0x1DC) != 0) count += 1;
        if (*(s32*)(arg0 + 0x1E4) != 0) count += 1;
        if (*(s32*)(arg0 + 0x1EC) != 0) count += 1;
        if (*(s32*)(arg0 + 0x1F4) != 0) count += 1;
    }
    return count;
}

// 0x800762A0
// Resets the object (fn_80070460), then sends event 0xCA90298E to the object at +0x5D4 and to the
// four groups at +0x5A4 / +0x5B4 / +0x5C4 (stride 4).
void fn_800762A0(u8* arg0) {
    fn_80070460(arg0);
    EventSend(gEventMgr, *(void**)(arg0 + 0x5D4), 0, 0xCA90298Eu, 0, 0, 1, 0);
    u8* cur = arg0;
    for (s32 i = 0; i < 4; i++) {
        EventSend(gEventMgr, *(void**)(cur + 0x5A4), 0, 0xCA90298Eu, 0, 0, 1, 0);
        EventSend(gEventMgr, *(void**)(cur + 0x5B4), 0, 0xCA90298Eu, 0, 0, 1, 0);
        EventSend(gEventMgr, *(void**)(cur + 0x5C4), 0, 0xCA90298Eu, 0, 0, 1, 0);
        cur += 4;
    }
}

// 0x800763BC
// Same event fan-out as fn_800762A0 with event 0xC608C842 and no reset.
void fn_800763BC(u8* arg0) {
    EventSend(gEventMgr, *(void**)(arg0 + 0x5D4), 0, 0xC608C842u, 0, 0, 1, 0);
    u8* cur = arg0;
    for (s32 i = 0; i < 4; i++) {
        EventSend(gEventMgr, *(void**)(cur + 0x5A4), 0, 0xC608C842u, 0, 0, 1, 0);
        EventSend(gEventMgr, *(void**)(cur + 0x5B4), 0, 0xC608C842u, 0, 0, 1, 0);
        EventSend(gEventMgr, *(void**)(cur + 0x5C4), 0, 0xC608C842u, 0, 0, 1, 0);
        cur += 4;
    }
}

// 0x8007649C
// Same event fan-out as fn_800762A0 with event 0x117741C3 and no reset.
void fn_8007649C(u8* arg0) {
    EventSend(gEventMgr, *(void**)(arg0 + 0x5D4), 0, 0x117741C3u, 0, 0, 1, 0);
    u8* cur = arg0;
    for (s32 i = 0; i < 4; i++) {
        EventSend(gEventMgr, *(void**)(cur + 0x5A4), 0, 0x117741C3u, 0, 0, 1, 0);
        EventSend(gEventMgr, *(void**)(cur + 0x5B4), 0, 0x117741C3u, 0, 0, 1, 0);
        EventSend(gEventMgr, *(void**)(cur + 0x5C4), 0, 0x117741C3u, 0, 0, 1, 0);
        cur += 4;
    }
}

// 0x8009B69C
// Creates the scene object: takes the position from the prop at fn_800E4D7C, sets four state flags
// on its sub-object, syncs its transform, sets up the sub-object at +0x94 and stores a random
// phase (fmod of 0 and 2*pi) at +0x2B4.
u8* fn_8009B69C(void) {
    // fn_800C2618 gets a stale r3 (pass 0); its r3 result is passed on to fn_800C2028 as the object.
    u8* obj = fn_800C2028((u8*)(uintptr_t)fn_800C2618(0, 0));
    f32 pos[3];
    s32 sp8;
    u8* prop = fn_800E4D7C(fn_8006FBDC());
    typedef void (*FnC)(void*, void*, void*, f32);
    (*(FnC*)(*(u8**)prop + 0xC))(prop, pos, &sp8, lbl_805FDCCC);
    u8* sub = *(u8**)(obj + 0x4);
    *(f32*)(sub + 0x0) = pos[0];
    *(f32*)(sub + 0x4) = pos[1];
    *(f32*)(sub + 0x8) = pos[2];
    *(s32*)(sub + 0x20) |= 0x8000;
    *(s32*)(sub + 0x20) |= 0x10000000;
    *(s32*)(sub + 0x20) |= 0x4000;
    *(s32*)(sub + 0x20) |= 0x10000000;
    EntitySyncTransform(obj, 0);
    fn_801EC080(obj + 0x94, obj);
    *(f32*)(obj + 0x2B4) = fn_8033F200((f32)lbl_805FDCD0, (f32)lbl_805FDCD8);
    return obj;
}

// 0x800ABEC4
// Resets a 0x2A8-byte object to its base vtable and default fields, runs the base cleanup
// (fn_80062CE0) and frees the block when del > 0. Returns the object.
u8* fn_800ABEC4(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        *(void**)(arg0 + 0x0) = lbl_8035DA50;
        *(s32*)(arg0 + 0x264) = -1;
        *(s32*)(arg0 + 0x268) = -1;
        *(f32*)(arg0 + 0x26C) = lbl_805FDFC0;
        *(f32*)(arg0 + 0x270) = lbl_805FDFC0;
        *(f32*)(arg0 + 0x274) = lbl_805FDFC0;
        *(f32*)(arg0 + 0x278) = lbl_805FDFC0;
        *(f32*)(arg0 + 0x27C) = lbl_805FDFC0;
        *(f32*)(arg0 + 0x280) = lbl_805FDFC0;
        *(f32*)(arg0 + 0x284) = lbl_805FDFC4;
        *(f32*)(arg0 + 0x288) = lbl_805FDFC0;
        *(f32*)(arg0 + 0x28C) = lbl_805FDFC0;
        *(f32*)(arg0 + 0x290) = lbl_805FDFC0;
        *(f32*)(arg0 + 0x294) = lbl_805FDFC0;
        *(f32*)(arg0 + 0x298) = lbl_805FDFC0;
        *(f32*)(arg0 + 0x29C) = lbl_805FDFC0;
        *(s32*)(arg0 + 0x2A0) = 0;
        fn_80062CE0(arg0, 0);
        if (arg1 > 0) operator_delete(arg0);
    }
    return arg0;
}

// 0x80207C5C
// Loop while lbl_805FBB6C is set: lock/unlock lbl_80556AB0 around a sleep of
// (bus clock / 4000) ticks, and clear +0xB4 once the flag is seen. Returns 0.
s32 fn_80207C5C(u8* arg0, s32 arg_sp0) {
    typedef s32 (*Fn50)(void*);
    lbl_805FBB68 = 1;
    s32 r = (*(Fn50*)(*(u8**)arg0 + 0x50))(arg0);
    lbl_805FB380 = 1;
    lbl_805FBB70 = r;
    lbl_805FBB64 = 0;
    while (lbl_805FBB6C != 0) {
        fn_8023A5B0(lbl_80556AB0);
        lbl_805FBB48 += 1;
        if (lbl_805FBB6C != 0 && *(s32*)(arg0 + 0xB4) > 0) {
            fn_802E93CC();
            *(s32*)(arg0 + 0xB4) = 0;
        }
        fn_8023A68C(lbl_80556AB0);
        lbl_805FBB48 -= 1;
        fn_8023D3C8(0, OSGetBusClock() / 4000);
    }
    lbl_805FBB68 = 0;
    return 0;
}

// 0x800CD59C
// Returns 1 when the tag at +0x9C -> +8 equals one of the four tag words lbl_805F8C78[0..1] or
// lbl_805F8C80[0..1], else 0. A missing sub-object reads as tag 0.
s32 fn_800CD59C(u8* arg0) {
    u8* sub = *(u8**)(arg0 + 0x9C);
    u32 tag = 0;
    if (sub != 0) tag = *(u32*)(sub + 0x8);
    if (tag == lbl_805F8C78[0] || tag == lbl_805F8C78[1] ||
        tag == lbl_805F8C80[0] || tag == lbl_805F8C80[1]) {
        return 1;
    }
    return 0;
}

// 0x8011A9E0
// Resets the list header, copies the record count from the save block (+0x10EC), and for each
// record calls fn_8011B060 with the block entry at +0xA58 and the record slot; then clears flags.
void fn_8011A9E0(u8* arg0, s32 arg_sp0) {
    *(s32*)(arg0 + 0x4) = 5;
    *(s32*)(arg0 + 0x2CB0) = 5;
    *(s32*)(arg0 + 0x2CAC) = 3;
    *(s32*)(arg0 + 0x4) = 1;
    u8* rec = arg0;
    u8* dst = arg0 + 0x1C;
    s32 i = 0;
    s32 off = 0;
    u8* save = *(u8**)lbl_805FBBE0;
    *(s32*)(arg0 + 0x25A4) = *(s32*)(save + 0x10EC);
    *(s32*)(arg0 + 0x2DE0) = 0;
    while (i < *(s32*)(arg0 + 0x25A4)) {
        u8* blk = *(u8**)(*(u8**)lbl_805FBBE0 + off + 0xA58);
        fn_8011B060(blk, dst);
        *(s32*)(rec + 0x70) = i;
        off += 4;
        dst += 0x60;
        rec += 0x60;
        i += 1;
    }
    *(s32*)(arg0 + 0x8) = 5;
    *(s32*)(arg0 + 0x25A0) = 0;
    lbl_805FB2A4[0] = 0;
    *(s32*)(arg0 + 0x2DE4) = -1;
}

// 0x801AFADC
// Sphere/plane test between two objects: returns 0 when the object at arg1 has +0xC set or when
// the centre distance is larger than the radius sum; otherwise returns 1 when the dot product of
// the offset with the plane vector at arg0 +0x40..+0x48 is at or below arg1 +0x3C.
s32 fn_801AFADC(u8* arg0, u8* arg1) {
    if (*(s32*)(arg1 + 0xC) != 0) return 0;
    f32 dx = *(f32*)(arg1 + 0x14) - *(f32*)(arg0 + 0x14);
    f32 dy = *(f32*)(arg1 + 0x18) - *(f32*)(arg0 + 0x18);
    f32 dz = *(f32*)(arg1 + 0x1C) - *(f32*)(arg0 + 0x1C);
    f32 rs = *(f32*)(arg1 + 0x3C) + *(f32*)(arg0 + 0x3C);
    f32 d2 = (dx * dx + dy * dy) + dz * dz;
    if (d2 - rs * rs > lbl_805FFAB8) return 0;
    f32 dot = dz * *(f32*)(arg0 + 0x48) + (dx * *(f32*)(arg0 + 0x40) + dy * *(f32*)(arg0 + 0x44));
    return (dot <= *(f32*)(arg1 + 0x3C)) ? 1 : 0;
}

// 0x80025C04
// Teardown: stores the vtable at +0x4, detaches the 15 texture slots at +0x1F4 (releasing each
// when its refcount reaches 0), frees the buffer at +0xB0, runs the sub-object teardown and
// deletes the object when del > 0. Returns the object.
u8* fn_80025C04(u8* arg0, s32 arg1) {
    if (arg0 != 0) {
        *(void**)(arg0 + 0x4) = lbl_80357408;
        fn_801B3378(fn_801B3308(), arg0 + 0xCC);
        lbl_805FAFB8 = 0;
        for (s32 i = 0; i < 0xF; i++) {
            u8* tex = *(u8**)(arg0 + 0x1F4 + i * 4);
            if (tex != 0) {
                s32 rc = *(s32*)(tex + 0x24) - 1;
                *(s32*)(tex + 0x24) = rc;
                if (rc == 0) TextureRelease(tex);
                *(u8**)(arg0 + 0x1F4 + i * 4) = 0;
            }
        }
        u8* buf = *(u8**)(arg0 + 0xB0);
        if (buf != 0) MemFreeSmall(buf - 0x10);
        fn_801F6238(arg0 + 0xCC, -1);
        fn_800216B8(arg0, 0);
        if (arg1 > 0) operator_delete(arg0);
    }
    return arg0;
}

// 0x8002C080
// Builds a segment record: copies the start point (arg1) and end point (arg2) into arg0, then the
// direction vector between them, its length (0 when the squared length is not above the
// constant) and the start point again as a copy at +0x1C..+0x24.
void fn_8002C080(u8* arg0, u8* arg1, u8* arg2) {
    f32 k = lbl_805FCB80;
    *(f32*)(arg0 + 0x4) = *(f32*)(arg1 + 0x0);
    *(f32*)(arg0 + 0x8) = *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0xC) = *(f32*)(arg1 + 0x8);
    *(f32*)(arg0 + 0x10) = *(f32*)(arg2 + 0x0);
    f32 dx = *(f32*)(arg2 + 0x0) - *(f32*)(arg1 + 0x0);
    *(f32*)(arg0 + 0x14) = *(f32*)(arg2 + 0x4);
    f32 dy = *(f32*)(arg2 + 0x4) - *(f32*)(arg1 + 0x4);
    *(f32*)(arg0 + 0x18) = *(f32*)(arg2 + 0x8);
    f32 dz = *(f32*)(arg2 + 0x8) - *(f32*)(arg1 + 0x8);
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 len = k;
    if (len2 > k) len = sqrtf(len2);  // asm: frsqrte + two Newton steps, i.e. sqrt
    *(f32*)(arg0 + 0x28) = len;
    *(f32*)(arg0 + 0x2C) = k;
    *(f32*)(arg0 + 0x1C) = *(f32*)(arg0 + 0x4);
    *(f32*)(arg0 + 0x20) = *(f32*)(arg0 + 0x8);
    *(f32*)(arg0 + 0x24) = *(f32*)(arg0 + 0xC);
}

}  // extern "C"
