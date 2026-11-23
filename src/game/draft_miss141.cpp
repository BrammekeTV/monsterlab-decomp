#include "types.h"

// Data symbols. gWorldState holds a pointer to the world block (deref with *(u8**)gWorldState).
extern "C++" s32 gZoneResourceSize;
extern "C++" u8 lbl_8035A760[];
extern u8 gWorldState[];
extern u8 lbl_8035A944[];
extern u8 lbl_8047C640[];
extern f32 lbl_803474D8[];
extern f32 lbl_803474E8[];
extern u8* lbl_803C6A58[3];
extern u8 lbl_80383B60[];
extern u8 lbl_803C4298[];
extern u8 lbl_805FB1A1;
extern f32 lbl_805FD5D8;
extern f32 lbl_805FD5DC;
extern f32 lbl_805FD658;
extern f32 lbl_805FD65C;
extern f32 lbl_805FD748;
extern f32 lbl_805FD74C;
extern f32 lbl_805FDB00;
extern f32 lbl_805FE348;
extern f32 lbl_805FE34C;
extern f32 lbl_805FEA54;

// 0x800C0B54 (C++ linkage in small_800C.cpp)
void fn_800C0B54(u8* p);

extern "C" {

// 0x80074CC8
u8* fn_80074CC8(u8* p, void* a);
// 0x801D8AD4
f32 fn_801D8AD4(u8* p);
// 0x80075ACC
void fn_80075ACC(u8* p);
// 0x80075E8C
void fn_80075E8C(u8* p);
// 0x801B3308
u8* fn_801B3308(void);
// 0x801B3310
void fn_801B3310(u8* p, u8* q);
// 0x80070458
s32 RaySceneTest(void* ray);
// 0x8032F1E8
void fn_8032F1E8(u8* arr, void (*ctor)(void), void (*dtor)(void), s32 size, s32 count);
// 0x800C82BC
void fn_800C82BC(void);
// 0x800C0BA4

u8* fn_800C0BA4(u8* p, s32 del);
// 0x800C6C88
void fn_800C6C88(u8* p);
// 0x800C6CA4
u8* fn_800C6CA4(u8* p, s32 del);
// 0x801ED538
s32 fn_801ED538(u8* p);
// 0x801B7198
void* fn_801B7198(void* anim, s32 a);
// 0x801AC184
u8* AnimDefFindByHandle(u8* defs, s32 idx);
// 0x8017493C
void fn_8017493C(void* p);
// 0x8018C4D8
void fn_8018C4D8(u8* p);
// 0x801F3918
void* EventPost(s32 id, void* sender);
// 0x801D4980
void BufferFree(void* p);
// 0x801D426C
void operator_delete(void* p);

// 0x8006FA74
// Base init for a world-object block: runs the base ctor, sets the type from arg1 and float defaults.
u8* fn_8006FA74(u8* arg0, s32 arg1) {
    fn_80074CC8(arg0, (void*)1);
    *(void**)(arg0 + 0x4) = lbl_8035A760;
    *(s32*)(arg0 + 0x68) = arg1;
    *(f32*)(arg0 + 0x6C) = lbl_805FD5D8;
    *(f32*)(arg0 + 0x70) = fn_801D8AD4(lbl_8047C640);
    *(f32*)(arg0 + 0x7C) = lbl_805FD5DC;
    *(f32*)(arg0 + 0x80) = lbl_805FD5DC;
    *(f32*)(arg0 + 0x84) = lbl_805FD5DC;
    *(f32*)(arg0 + 0x88) = lbl_805FD5D8;
    return arg0;
}

// 0x8006FC04
// Clears bit 0 of the flags word (+0x30) on every entry of every zone's entry table.
void fn_8006FC04(void) {
    s32 zone;
    for (zone = 0; zone <= *(s32*)(*(u8**)gWorldState + 0x4); zone++) {
        u8* zoneRes = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
        u8 i = 0;
        while (i < *(u8*)(zoneRes + 0xA4)) {
            u8* entry = *(u8**)(zoneRes + 0xA0) + i * 0x4C;
            i++;
            *(s32*)(entry + 0x30) &= ~1;
        }
    }
}

// 0x800759E8
// Sets up the two sub-object blocks (+0x5E0 and +0x1E4 offsets) with their list pointers and float params.
void fn_800759E8(u8* arg0) {
    fn_80075ACC(arg0);
    fn_80075E8C(arg0);
    *(void**)(arg0 + 0x5E0) = lbl_8035A944;
    *(s32*)(arg0 + 0x60C) = 0;
    *(void**)(arg0 + 0x618) = lbl_8035A944 + 3;
    *(s32*)(arg0 + 0x644) = 1;
    *(void**)(arg0 + 0x650) = lbl_8035A944 + 6;
    *(s32*)(arg0 + 0x67C) = 2;
    *(void**)(arg0 + 0x688) = lbl_8035A944 + 9;
    *(s32*)(arg0 + 0x6B4) = 3;
    *(s32*)(arg0 + 0x124) = 0;
    *(f32*)(arg0 + 0x138) = lbl_805FD658;
    *(f32*)(arg0 + 0x140) = lbl_805FD65C;
    *(s32*)(arg0 + 0x13C) = 3;
    *(s32*)(arg0 + 0x144) = 0x28;
    fn_801B3310(fn_801B3308(), arg0 + 0x8C);
    *(s32*)(arg0 + 0x1E4) = 1;
    *(f32*)(arg0 + 0x1F8) = lbl_805FD658;
    *(f32*)(arg0 + 0x200) = lbl_805FD65C;
    *(s32*)(arg0 + 0x1FC) = 3;
    *(s32*)(arg0 + 0x204) = 0x28;
    fn_801B3310(fn_801B3308(), arg0 + 0x14C);
}

// 0x8007CED8
// Resets the ray-probe block and stores the two per-hit-type float values looked up from RaySceneTest.
u8* fn_8007CED8(u8* arg0) {
    s32 hit;
    *(f32*)(arg0 + 0x0) = lbl_805FD748;
    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xC) = 0;
    *(s32*)(arg0 + 0x10) = 0;
    *(f32*)(arg0 + 0x14) = lbl_805FD74C;
    *(s32*)(arg0 + 0x20) = 0;
    hit = RaySceneTest(arg0);
    *(f32*)(arg0 + 0x18) = lbl_803474D8[hit];
    *(f32*)(arg0 + 0x1C) = lbl_803474E8[hit];
    return arg0;
}

// 0x80091D28
// Releases a slot in the door table, clears the block and frees it when arg1 > 0.
void* fn_80091D28(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        s32 idx = *(s32*)(arg0 + 0x8);
        if (idx != -1) {
            lbl_803C6A58[idx] = NULL;
        }
        *(s32*)(arg0 + 0x0) = 0;
        *(s32*)(arg0 + 0x4) = -1;
        *(s32*)(arg0 + 0x8) = -1;
        *(u8*)(arg0 + 0xC) = 0;
        *(f32*)(arg0 + 0x10) = lbl_805FDB00;
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x800BF97C
// Initialises a block: sets the -1 state, runs the array setup with the two callbacks and zeroes the float fields.
void fn_800BF97C(u8* arg0) {
    *(s32*)(arg0 + 0x0) = -1;
    fn_8032F1E8(arg0 + 0x14, (void (*)(void))fn_800C0B54, (void (*)(void))fn_800C0BA4, 0x48, 4);
    *(f32*)(arg0 + 0x134) = lbl_805FE348;
    *(f32*)(arg0 + 0x138) = lbl_805FE348;
    *(f32*)(arg0 + 0x13C) = lbl_805FE348;
    *(f32*)(arg0 + 0x140) = lbl_805FE348;
    *(f32*)(arg0 + 0x144) = lbl_805FE348;
    *(f32*)(arg0 + 0x148) = lbl_805FE348;
    *(f32*)(arg0 + 0x14C) = lbl_805FE348;
    *(f32*)(arg0 + 0x150) = lbl_805FE348;
    *(f32*)(arg0 + 0x154) = lbl_805FE348;
    *(f32*)(arg0 + 0x158) = lbl_805FE348;
    *(f32*)(arg0 + 0x15C) = lbl_805FE348;
    *(f32*)(arg0 + 0x160) = lbl_805FE348;
    *(f32*)(arg0 + 0x164) = lbl_805FE34C;
    *(f32*)(arg0 + 0x168) = lbl_805FE348;
    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xC) = 0;
    *(s32*)(arg0 + 0x10) = 0;
}

// 0x800BFAC0
// Sets the position from arg1 and, if an owner object is attached, copies its offset and sets four state flags on it.
void fn_800BFAC0(u8* arg0, f32* arg1) {
    u8* owner;
    u8* obj;
    *(s32*)(arg0 + 0x0) = 0;
    *(f32*)(arg0 + 0x134) = arg1[0];
    *(f32*)(arg0 + 0x138) = arg1[1];
    *(f32*)(arg0 + 0x13C) = arg1[2];
    *(f32*)(arg0 + 0x140) = lbl_805FE348;
    *(f32*)(arg0 + 0x144) = lbl_805FE348;
    *(f32*)(arg0 + 0x148) = lbl_805FE348;
    *(f32*)(arg0 + 0x14C) = lbl_805FE348;
    *(f32*)(arg0 + 0x150) = lbl_805FE348;
    *(f32*)(arg0 + 0x154) = lbl_805FE348;
    *(f32*)(arg0 + 0x158) = lbl_805FE348;
    *(f32*)(arg0 + 0x15C) = lbl_805FE348;
    *(f32*)(arg0 + 0x160) = lbl_805FE348;
    *(f32*)(arg0 + 0x164) = lbl_805FE34C;
    *(f32*)(arg0 + 0x168) = lbl_805FE348;
    owner = *(u8**)(arg0 + 0x4);
    if (owner != NULL) {
        obj = *(u8**)(owner + 0x4);
        *(f32*)(obj + 0x0) = *(f32*)(owner + 0xC8);
        *(f32*)(obj + 0x4) = *(f32*)(owner + 0xCC);
        *(f32*)(obj + 0x8) = *(f32*)(owner + 0xD0);
        obj = *(u8**)(owner + 0x4);
        *(s32*)(obj + 0x20) |= 0x8000;
        obj = *(u8**)(owner + 0x4);
        *(s32*)(obj + 0x20) |= 0x10000000;
        obj = *(u8**)(owner + 0x4);
        *(s32*)(obj + 0x20) |= 0x4000;
        obj = *(u8**)(owner + 0x4);
        *(s32*)(obj + 0x20) |= 0x10000000;
    }
}

// 0x800C749C
// Runs the sub-system init, then sets up the three 0x64-entry arrays with their ctor/dtor and clears four counters.
void fn_800C749C(u8* arg0) {
    fn_800C82BC();
    fn_8032F1E8(arg0 + 0x14, (void (*)(void))fn_800C6C88, (void (*)(void))fn_800C6CA4, 0x14, 0x64);
    fn_8032F1E8(arg0 + 0x7E4, (void (*)(void))fn_800C6C88, (void (*)(void))fn_800C6CA4, 0x14, 0x64);
    fn_8032F1E8(arg0 + 0xFB4, (void (*)(void))fn_800C6C88, (void (*)(void))fn_800C6CA4, 0x14, 0x64);
    *(s32*)(arg0 + 0x1784) = 0;
    *(s32*)(arg0 + 0x1788) = 0;
    *(s32*)(arg0 + 0x178C) = 0;
    *(s32*)(arg0 + 0x2F04) = 0;
}

// 0x800E4CB0
// Looks up the anim for the handle at +0x58; if an index is set at +0x5C, flags the item and fires the state callbacks.
void fn_800E4CB0(u8* arg0) {
    s32 handle;
    u8* anim;
    u8* item;
    u8* b;
    handle = fn_801ED538(arg0 + 0x58);
    anim = (u8*)fn_801B7198(&handle, 0);
    if (anim != NULL) {
        if (*(s32*)(arg0 + 0x5C) != 0) {
            item = AnimDefFindByHandle(*(u8**)(*(u8**)(anim + 0x9C) + 0x48), *(s32*)(arg0 + 0x5C));
            *(s32*)(item + 0xC) |= 0x10000;
            (*(void (**)(u8*, s32))(*(u8**)anim + 0x84))(anim, *(s32*)(arg0 + 0x5C));
            lbl_805FB1A1 = 0;
            b = *(u8**)(lbl_803C4298 + 0x4);
            (*(void (**)(u8*))(*(u8**)b + 0x10))(b);
        }
        (*(void (**)(u8*, f32))(*(u8**)anim + 0x90))(anim, lbl_805FEA54);
    }
}

// 0x801ACB98
// Clears the count at +0x0 and frees the buffer at +0x4 if present.
void fn_801ACB98(u8* arg0) {
    if (*(s32*)(arg0 + 0x0) != 0) {
        *(s32*)(arg0 + 0x0) = 0;
    }
    if (*(void**)(arg0 + 0x4) != NULL) {
        BufferFree(*(void**)(arg0 + 0x4));
        *(void**)(arg0 + 0x4) = NULL;
    }
}

// 0x801ACCA0
// Deleting destructor: sets the vtable, frees the object when arg1 > 0, returns the object.
void* fn_801ACCA0(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(void**)arg0 = lbl_80383B60;
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// 0x801AC7F0
// Clears up to three 0x24-byte slots at arg0+4 (freeing their payload) and posts event 0x7C to arg1 unless its flag 0x04000000 is set.
void fn_801AC7F0(u8* arg0, u8* arg1) {
    s32 n = 0;
    u8* slot = arg0 + 0x4;
    while (n < 3 && ((u32)*(s32*)slot + 0x10000u) != 0xFFFFu) {
        u8* payload = *(u8**)(slot + 0x4);
        if (payload != NULL) {
            fn_8017493C(payload);
        }
        *(void**)(slot + 0x4) = NULL;
        *(s32*)(slot + 0x0) = -1;
        fn_8018C4D8(slot);
        slot += 0x24;
        n++;
    }
    if (!(*(s32*)(*(u8**)(arg1 + 0x4) + 0x20) & 0x04000000)) {
        u8* ev = (u8*)EventPost(0x7C, arg1);
        if (ev != NULL) {
            *(s32*)(ev + 0x4) = -2;
        }
    }
}

}  // extern "C"
