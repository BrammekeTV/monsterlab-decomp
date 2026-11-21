// Small functions in 0x8014xxxx-0x8016xxxx: smasher record lookup and effects, render-thread queue
// waits, effect slot flushing, texture refcount release, and global object resets.
#include "types.h"
#include "revolution.h"
#include "engine/rend.h"  // gRendMem (C++ linkage, defined in rend_init.cpp)

extern "C" {
void*  MemAllocSmall(u32 size);                                                 // 0x801D41CC
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner); // 0x801C80E0
s32    RaySceneTest(void* ray);                                                 // 0x80070458
void   TextureRelease(void* tex);                                               // 0x80184028
void   fn_80173164(void* p);                                                    // 0x80173164
void   fn_801731C8(void* p);                                                    // 0x801731C8
void   fn_8016A2B8(u8* p);                                                      // 0x8016A2B8
void   fn_8016992C(void* p);                                                    // 0x8016992C
void   fn_80160AFC(u8* p);                                                      // 0x80160AFC
void   fn_801604F4(u8* p);                                                      // 0x801604F4 (asm passes lbl_8040D500)
void   fn_80188E14(void);                                                       // 0x80188E14
void   fn_801483CC(u8* a3);                                                     // 0x801483CC
u8*    fn_801483FC(u8* obj, s32 del);                                                      // 0x801483FC (address only)
void*  fn_8032F028(void* mem, void* ctor, void* a, s32 size, s32 count);        // 0x8032F028
extern u8*  lbl_803C6A88[3];       // smasher record table (key at +4)
extern u8   lbl_8047C760[];        // effect manager
extern f32  lbl_805FDB20;          // float constant (w component of the effect position)
extern f32  lbl_805FD650;          // float constant (lower clamp)
extern f32  lbl_80347280[];        // float table indexed by RaySceneTest
extern void* lbl_805FB4A0;         // particle pool block pointer
extern s32  lbl_805FB544;          // dispatch table count
extern u8   lbl_805FB548[];        // dispatch table keys
extern u8   lbl_8040AE88[];        // dispatch table handlers
extern s32  lbl_805FB560;          // render request index
extern u32  lbl_805FB570;
extern u32  lbl_805FB574;
extern u8   sRendThread[];         // render-thread request queue
extern u8   lbl_8040D500[];
extern u8   lbl_8040E260[];
extern u8   lbl_8040E5F0[];        // frame effects object
extern u8*  lbl_805FB5A8;
extern void* lbl_805FB5AC;         // particle list pending an update outside zones
extern u8*  lbl_805FB608;          // global object pointer
extern void* lbl_805FB5DC;
extern s32  lbl_805FB5B8;
extern u8   lbl_804126D8[];
extern u8   lbl_804128D8[];
}

extern "C" {

// Finds the smasher record whose key (+4) matches key in the 3-entry table, or NULL.
static u8* RobotEvolutionExperiment_FindSmasher(s32 key) {
    s32 i;
    for (i = 0; i < 3; i++) {
        u8* rec = lbl_803C6A88[i];
        if (*(s32*)(rec + 0x4) == key) {
            return rec;
        }
    }
    return NULL;
}

// Spawns a smasher effect at the record's position (record +0xC holds the position block).
static void RobotEvolutionExperiment_SpawnSmasherEffect(u8* arg0, u32 effect) {
    u8* rec = RobotEvolutionExperiment_FindSmasher(*(s32*)(arg0 + 0x10));
    if (rec != NULL) {
        u8* posBlock = *(u8**)(rec + 0xC);
        f32 pos[4];
        pos[0] = *(f32*)(posBlock + 0x8);
        pos[1] = *(f32*)(posBlock + 0xC);
        pos[2] = *(f32*)(posBlock + 0x10);
        pos[3] = lbl_805FDB20;
        SpawnEffectAt(lbl_8047C760, effect, pos, 0);
    }
}

// 0x80093E5C
// Smasher hit: spawns the hit effect at the smasher's position.
void RobotEvolutionExperiment_SmasherHit(u8* arg0) {
    RobotEvolutionExperiment_SpawnSmasherEffect(arg0, 0xF7A1EDD6u);
}

// 0x80093F0C
// Smasher miss: spawns the miss effect at the smasher's position.
void RobotEvolutionExperiment_SmasherMiss(u8* arg0) {
    RobotEvolutionExperiment_SpawnSmasherEffect(arg0, 0x1166D3AFu);
}

// 0x800765D0
// Advances the ring index of slot arg1 (stride 0x38) and, when its entry is clear, adds a
// table-driven value to the float at +0x6BC, clamped at lbl_805FD650.
void fn_800765D0(u8* arg0, s32 arg1) {
    u8* slot = arg0 + arg1 * 0x38;
    s32 idx = *(s32*)(slot + 0x604);
    u8 flag = *(u8*)(slot + idx * 8 + 0x5E4);
    *(s32*)(slot + 0x604) = (idx + 1) & 3;
    if (flag == 0) {
        s32 sel = RaySceneTest(NULL);
        f32 v = *(f32*)(arg0 + 0x6BC) + lbl_80347280[sel];
        *(f32*)(arg0 + 0x6BC) = v;
        if (v < lbl_805FD650) {
            *(f32*)(arg0 + 0x6BC) = lbl_805FD650;
        }
    }
}

// 0x80148314
// Allocates the 4-entry particle pool (stride 0x1C) with arg0 slots of 0x84 bytes each.
void fn_80148314(s32 arg0, u8* arg1) {
    s32 i = 0;
    s32 off = 0;
    lbl_805FB4A0 = fn_8032F028(MemAllocSmall(0x80), (void*)fn_801483CC, (void*)fn_801483FC, 0x1C, 4);
    do {
        u8* e = (u8*)lbl_805FB4A0 + off;
        *(s32*)(e + 0xC) = arg0;
        *(void**)(e + 0x4) = MemAllocSmall(arg0 * 0x84);
        i += 1;
        *(void**)(e + 0x8) = MemAllocSmall(*(s32*)(e + 0xC) * 4);
        off += 0x1C;
    } while (i < 4);
}

// 0x8015AAE8
// Frees the two owned blocks at +0x18 and +0x1C (the latter only when the flag at +0xD is set)
// and clears the low flag bit at +0xB.
void fn_8015AAE8(void* arg0) {
    u8* p = (u8*)arg0;
    void* a = *(void**)(p + 0x18);
    if (a != NULL) {
        gRendMem->free(a);
    }
    *(void**)(p + 0x18) = NULL;
    if (*(u8*)(p + 0xD) != 0) {
        gRendMem->free(*(void**)(p + 0x1C));
        *(void**)(p + 0x1C) = NULL;
    }
    *(u8*)(p + 0xB) &= 0xFE;
}

// 0x8015EA90
// Waits until the render request queue has 0x40 free slots and the done counter catches up
// with the submitted counter.
void fn_8015EA90(void) {
    u8* rt = sRendThread;
    while (OSGetSemaphoreCount((OSSemaphore*)(rt + 0x2618)) < 0x40) {
        OSWaitSemaphore((OSSemaphore*)(rt + 0x2630));
    }
    while (lbl_805FB574 != lbl_805FB570) {
        OSWaitSemaphore((OSSemaphore*)(rt + 0x263C));
    }
}

// Flushes one pending slot of obj: when the flag halfword at p+valOff+4 is set, moves the
// value into the 0x380 free list (low 5 bits) and clears the flag.
static void RobotEvolution_FlushSlot(u8* obj, u8* p, s32 valOff) {
    if (*(s16*)(p + valOff + 0x4) == 0) {
        return;
    }
    s32 v = *(s32*)(p + valOff) + 0x20;
    *(s32*)(p + valOff) = v;
    *(s16*)(p + valOff + 0x4) = 0;
    *(s16*)(p + valOff + 0x6) = (s16)*(s32*)(obj + 0x380);
    *(s32*)(obj + 0x380) = v & 0x1F;
}

// 0x80160A28
// Flushes the four pending slots of each of the 8 entries (stride 0x70) of arg0.
// Returns arg0 unchanged.
u8* fn_80160A28(u8* arg0) {
    u8* p = arg0;
    s32 i;
    for (i = 0; i < 8; i++) {
        RobotEvolution_FlushSlot(arg0, p, 0x00);
        RobotEvolution_FlushSlot(arg0, p, 0x1C);
        RobotEvolution_FlushSlot(arg0, p, 0x38);
        RobotEvolution_FlushSlot(arg0, p, 0x54);
        p += 0x70;
    }
    return arg0;
}

// 0x80162B6C
// Returns 1 if n matches the id at +0xC4/+0x19C/+0x274/+0x34C of any of the 16 entries (stride
// 0x360) with its state (+0xD0/+0x1A8/+0x280/+0x358) in 1..3, else 0.
s32 fn_80162B6C(void* obj, s32 n) {
    u8* p = (u8*)obj;
    s32 i;
    for (i = 0; i < 0x10; i++) {
        if (*(s32*)(p + 0xC4) == n && (u32)(*(s32*)(p + 0xD0) - 1) <= 2U) {
            return 1;
        }
        if (*(s32*)(p + 0x19C) == n && (u32)(*(s32*)(p + 0x1A8) - 1) <= 2U) {
            return 1;
        }
        if (*(s32*)(p + 0x274) == n && (u32)(*(s32*)(p + 0x280) - 1) <= 2U) {
            return 1;
        }
        if (*(s32*)(p + 0x34C) == n && (u32)(*(s32*)(p + 0x358) - 1) <= 2U) {
            return 1;
        }
        p += 0x360;
    }
    return 0;
}

// 0x80162C28
// Sets the per-slot flag bytes (+4 after each state word equal to 1 or 2) across the 8 entries
// (stride 0x6C0) of lbl_8040E5F0, then runs the two init helpers.
void fn_80162C28(void) {
    u8* e = lbl_8040E5F0;
    s32 i;
    for (i = 0; i < 8; i++) {
        if ((u32)(*(s32*)(e + 0xD0) - 1) <= 1U) *(u8*)(e + 0xD4) = 1;
        if ((u32)(*(s32*)(e + 0x1A8) - 1) <= 1U) *(u8*)(e + 0x1AC) = 1;
        if ((u32)(*(s32*)(e + 0x280) - 1) <= 1U) *(u8*)(e + 0x284) = 1;
        if ((u32)(*(s32*)(e + 0x358) - 1) <= 1U) *(u8*)(e + 0x35C) = 1;
        if ((u32)(*(s32*)(e + 0x430) - 1) <= 1U) *(u8*)(e + 0x434) = 1;
        if ((u32)(*(s32*)(e + 0x508) - 1) <= 1U) *(u8*)(e + 0x50C) = 1;
        if ((u32)(*(s32*)(e + 0x5E0) - 1) <= 1U) *(u8*)(e + 0x5E4) = 1;
        if ((u32)(*(s32*)(e + 0x6B8) - 1) <= 1U) *(u8*)(e + 0x6BC) = 1;
        e += 0x6C0;
    }
    fn_80160AFC(lbl_8040E260);
    fn_801604F4(lbl_8040D500);
}

// 0x80169704
// If the owner of arg1 (its +0x30 pointer minus 4) is arg0, releases arg1's header block and
// drops its refcount at +0x24, calling TextureRelease when it reaches zero.
void fn_80169704(void* arg0, void* arg1) {
    u8* p = (u8*)arg1;
    u8* data = *(u8**)(p + 0x30);
    u8* owner = data ? data - 4 : NULL;
    if (owner == arg0) {
        fn_801731C8(p + 0x28);
        s32 rc = *(s32*)(p + 0x24) - 1;
        *(s32*)(p + 0x24) = rc;
        if (rc == 0) {
            TextureRelease(p);
        }
    }
}

// 0x80169854
// If the owner of arg1 (its +0xC pointer minus 0x1C) is arg0, releases arg1's header block and
// drops its refcount at +0x10, calling fn_8016A2B8 when it reaches zero.
void fn_80169854(void* arg0, void* arg1) {
    u8* p = (u8*)arg1;
    u8* data = *(u8**)(p + 0xC);
    u8* owner = data ? data - 0x1C : NULL;
    if (owner == arg0) {
        fn_801731C8(p + 0x4);
        s32 rc = *(s32*)(p + 0x10) - 1;
        *(s32*)(p + 0x10) = rc;
        if (rc == 0) {
            fn_8016A2B8(p);
        }
    }
}

// 0x8016A254
// Detaches the global object at lbl_805FB5AC, releases it, clears its first word and calls the
// destructor slot (+4) of lbl_805FB608.
void fn_8016A254(void) {
    void* obj = lbl_805FB5AC;
    lbl_805FB5A8 = NULL;
    lbl_805FB5AC = NULL;
    fn_8016992C(obj);
    *(s32*)obj = 0;
    (*(void (**)(void*, void*))(lbl_805FB608 + 0x4))(obj, lbl_805FB608);
}

// 0x8016B7AC
// Zeroes the 0xC0-byte block at lbl_804128D8, lbl_805FB5B8 and the 16-byte block at lbl_804126D8.
void fn_8016B7AC(void) {
    s32 i;
    for (i = 0; i < 0x30; i++) {
        *(s32*)(lbl_804128D8 + i * 4) = 0;
    }
    lbl_805FB5B8 = 0;
    *(s32*)(lbl_804126D8 + 0x0) = 0;
    *(s32*)(lbl_804126D8 + 0xC) = 0;
    *(s32*)(lbl_804126D8 + 0x4) = 0;
    *(s32*)(lbl_804126D8 + 0x8) = 0;
}

// 0x8016E2CC
// Releases the object at lbl_805FB5DC, clears the pointer and runs fn_80188E14.
void fn_8016E2CC(void) {
    fn_80173164(lbl_805FB5DC);
    lbl_805FB5DC = NULL;
    fn_80188E14();
}

}  // extern "C"
