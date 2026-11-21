// Small functions from the miss491 batch: deleting destructors and teardown helpers,
// save/world-state queries, a projectile fire helper and two resets of global tables.
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*DestroyFn)(void* obj, s32 del);

extern "C" {
void  operator_delete(void* p);                                 // 0x801D426C
u8*   fn_801B3308(void);                                        // 0x801B3308
void  fn_801B3378(u8* p, u8* key);                              // 0x801B3378
u8*   fn_801F6238(u8* p, s32 del);                              // 0x801F6238
u8*   fn_801F81E0(u8* p, s32 del);                              // 0x801F81E0
u8*   fn_801F9598(u8* p, s32 del);                              // 0x801F9598
u8*   fn_801FA048(u8* p, s32 del);                              // 0x801FA048
u8*   fn_800216B8(u8* p, s32 del);                              // 0x800216B8
void  fn_801ABC50(void* h);                                     // 0x801ABC50
void  fn_800C7620(u8* p);                                       // 0x800C7620
u8*   fn_800C6CA4(u8* p, s32 del);                              // 0x800C6CA4
u8*   fn_800C82C0(u8* p, s32 del);                              // 0x800C82C0
void  __destroy_arr(void* arr, DestroyFn dtor, u32 size, u32 count); // 0x8032F2E0
void  fn_8003DCE0(u8* p);                                       // 0x8003DCE0
void  fn_8004C718(u8* p, s32 idx, void* elem);                  // 0x8004C718
void  fn_8005C480(u8* p);                                       // 0x8005C480 (no repo prototype; result unused)
void  fn_8020E59C(u8* p, s32 del);                              // 0x8020E59C
u8*   SaveGetCurrentMonster(u8* s);                             // 0x8022E170
void  fn_80023160(void);                                        // 0x80023160
void* fn_80060A28(u8* p);                                       // 0x80060A28
void  fn_800C2174(u8* a3, void* a4, s32 a5);                    // 0x800C2174

extern u8  gWorldState[];                                       // 0x803E6D00
extern u8  lbl_80357880[];                                      // 0x80357880
extern u8  lbl_803C6B60[];                                      // 0x803C6B60
extern u8  lbl_803C6B90[];                                      // 0x803C6B90
extern u8* lbl_805FAFD8;                                        // 0x805FAFD8
extern u8* lbl_805FBBE0;                                        // 0x805FBBE0
extern s32 lbl_805FDCB0;                                        // 0x805FDCB0
extern s32 lbl_805FDCB4;                                        // 0x805FDCB4
extern s32 lbl_805FDCB8;                                        // 0x805FDCB8
extern s32 lbl_805FDCBC;                                        // 0x805FDCBC
extern s32 lbl_805FDCC0;                                        // 0x805FDCC0
extern s32 lbl_805FDCC8;                                        // 0x805FDCC8
extern f32 lbl_805FCDBC;                                        // 0x805FCDBC (float constant, 0.0)
extern f32 lbl_805FCDE0;                                        // 0x805FCDE0 (float constant, 0.5)
extern f32 lbl_805FDCCC;                                        // 0x805FDCCC (float constant)
extern f32 lbl_805FDCE0;                                        // 0x805FDCE0 (float constant)

// 0x8002B8C4
// Deleting destructor: clears +0x48, tears down the sub-objects at +0x4D8 and +0x54, frees if del > 0.
u8* fn_8002B8C4(u8* p, s32 del) {
    if (p != NULL) {
        *(s32*)(p + 0x48) = 0;
        fn_801B3378(fn_801B3308(), p + 0x4D8);
        fn_801B3378(fn_801B3308(), p + 0x54);
        fn_801F6238(p + 0x4D8, -1);
        fn_801F81E0(p + 0x54, -1);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x800D7FB0
// Deleting destructor with no teardown: frees p if del > 0.
u8* fn_800D7FB0(u8* p, s32 del) {
    if (p != NULL && del > 0) operator_delete(p);
    return p;
}

// 0x8010FD2C
// Deleting destructor: clears the word at +0x4 if set, frees if del > 0.
u8* fn_8010FD2C(u8* p, s32 del) {
    if (p != NULL) {
        if (*(s32*)(p + 0x4) != 0) *(s32*)(p + 0x4) = 0;
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x8019B684
// Releases the handle at +0x10 through fn_801ABC50 and clears it. Returns 0.
s32 fn_8019B684(u8* p) {
    void* h = *(void**)(p + 0x10);
    if (h != NULL) {
        fn_801ABC50(h);
        *(void**)(p + 0x10) = NULL;
    }
    return 0;
}

// 0x8004D1A0
// Releases the five sub-objects at +0xC50..+0xC60, the one at +0xC4C, clears the flag at +0xC84,
// then calls fn_8005C480 on the object.
void fn_8004D1A0(u8* p) {
    for (s32 i = 0; i < 5; i++) {
        u8* sub = *(u8**)(p + 0xC50 + i * 4);
        if (sub != NULL) {
            fn_8004C718(p, i, NULL);
            fn_8003DCE0(sub);
        }
    }
    u8* obj = *(u8**)(p + 0xC4C);
    if (obj != NULL) {
        fn_8020E59C(obj, 1);
        *(void**)(p + 0xC4C) = NULL;
    }
    *(u8*)(p + 0xC84) = 0;
    fn_8005C480(p);
}

// 0x8009BD6C
// Resets the float triple at lbl_803C6B90 and registers six entries in the table at lbl_803C6B60.
void fn_8009BD6C(void) {
    *(f32*)(lbl_803C6B90 + 0x0) = lbl_805FDCCC;
    *(f32*)(lbl_803C6B90 + 0x4) = lbl_805FDCE0;
    *(f32*)(lbl_803C6B90 + 0x8) = lbl_805FDCCC;
    fn_800C2174(lbl_803C6B60,        &lbl_805FDCB0, 1);
    fn_800C2174(lbl_803C6B60 + 0x8,  &lbl_805FDCB4, 1);
    fn_800C2174(lbl_803C6B60 + 0x10, &lbl_805FDCB8, 1);
    fn_800C2174(lbl_803C6B60 + 0x18, &lbl_805FDCBC, 1);
    fn_800C2174(lbl_803C6B60 + 0x20, &lbl_805FDCC0, 2);
    fn_800C2174(lbl_803C6B60 + 0x28, &lbl_805FDCC8, 1);
}

// 0x8002CB58
// Deleting destructor: stores lbl_80357880 at +0x4, tears down the sub-objects at +0x21C and +0x31C,
// clears +0x9C and lbl_805FAFD8, runs fn_800216B8 on the object, frees if del > 0. Returns p.
u8* fn_8002CB58(u8* p, s32 del) {
    if (p != NULL) {
        *(u8**)(p + 0x4) = lbl_80357880;
        fn_801B3378(fn_801B3308(), p + 0x21C);
        fn_801B3378(fn_801B3308(), p + 0x31C);
        *(s32*)(p + 0x9C) = 0;
        lbl_805FAFD8 = NULL;
        fn_801F9598(p + 0x31C, -1);
        fn_801FA048(p + 0x21C, -1);
        fn_800216B8(p, 0);
        if (del > 0) operator_delete(p);
    }
    return p;
}

// 0x80036574
// Calls fn_80023160 once when the float at +0x6C is in (0, 0.5 * [+0x78]), the sum of the words at
// +0x424, +0x364 and +0x254 is below half of [+0x5F4] (signed compare), and the flag at +0x634 is clear.
void fn_80036574(u8* p) {
    u32 cap = *(u32*)(p + 0x5F4);
    s32 half = ((s32)((cap >> 31) + cap)) >> 1;
    s32 sum = *(s32*)(p + 0x424) + (*(s32*)(p + 0x364) + *(s32*)(p + 0x254));
    f32 v = *(f32*)(p + 0x6C);
    if (v > lbl_805FCDBC && v < *(f32*)(p + 0x78) * lbl_805FCDE0 && *(u8*)(p + 0x634) == 0 && sum < half) {
        fn_80023160();
        *(u8*)(p + 0x634) = 1;
    }
}

// 0x80062944
// Lazily creates the target at +0xA14. If the target's vtable slot 0x4C returns 0, fires a projectile
// at it, with the value read from *arg1 as the payload.
void fn_80062944(u8* p, s32* arg1, u32 arg2, f32 farg0) {
    if (*(void**)(p + 0xA14) == NULL) {
        *(void**)(p + 0xA14) = fn_80060A28(p);
    }
    u8* obj = *(u8**)(p + 0xA14);
    if (obj != NULL) {
        s32 (*vf)(void*) = *(s32 (**)(void*))(*(u8**)obj + 0x4C);
        if (vf(obj) == 0) {
            s32 sp8 = *arg1;
            FireProjectileAtEntity(p, arg2, *(void**)(p + 0xA14), (u32)(uintptr_t)&sp8, farg0);  // header prototype takes u32 for the pointer; truncates on x64
        }
    }
}

// 0x80065F40
// Returns bit 12 of the word at +0x838 of the world block's +0x8 sub-object. The asm also computes a
// squared distance from the object's position at +0x4 to a world point, but that result is dead
// (only cr0 from the final extrwi. is used). The bit is left in cr0 in the original, not in r3.
s32 fn_80065F40(u8* p) {
    (void)p;
    u8* world = *(u8**)gWorldState;
    u32 flags = *(u32*)(*(u8**)(world + 0x8) + 0x838);
    return (s32)((flags >> 12) & 1);
}

// 0x8011712C
// Fills the monster-index block at arg0 from the current save: clears four words and +0x50, stores the
// current monster, finds its slot in the list at lbl_805FBBE0+0xA30 (count at +0x6B8, -1 if absent),
// and stores the save word at +0x10E8 plus slot+1.
void fn_8011712C(u8* p) {
    *(s32*)(p + 0x24) = 0;
    *(s32*)(p + 0x28) = 0;
    *(s32*)(p + 0x38) = 0;
    *(s32*)(p + 0x3C) = 0;
    *(s32*)(p + 0x50) = 0;
    u8* cur = SaveGetCurrentMonster(lbl_805FBBE0);
    *(u8**)(p + 0x20) = cur;
    u8* list = lbl_805FBBE0 + 0xA30;
    s32 count = *(s32*)(list + 0x6B8);
    s32 slot = -1;
    for (s32 i = 0; i < count; i++) {
        if (*(u8**)(list + i * 4) == cur) {
            slot = i;
            break;
        }
    }
    *(s32*)(p + 0x2C) = slot;
    *(s32*)(p + 0x30) = slot;
    s32 saved = *(s32*)(lbl_805FBBE0 + 0x10E8);
    *(s32*)(p + 0x34) = saved;
    *(s32*)(p + 0x8) = saved;
    *(s32*)(p + 0x4) = slot + 1;
}

}  // extern "C"
