// The battle object: construction (tuning values from the game database), clearing between
// battles, and destruction; plus the random generator and block pool it owns.
// Original: 0x8020F8E0 - 0x8020FEEC, 0x80213660, 0x80220688 - 0x80220A4C, 0x8032F2E0
#include "types.h"
#include "game/part.h"

extern "C" {
void  operator_delete(void* p);                         // 0x801D4980
void* operator_new(u32 size);                           // 0x801D40E0
void* MemAllocSmall(u32 size);                          // 0x801D41CC
void  MemFreeSmall(void* p);                            // 0x801D4340
u8*   RoundSideDestroy(u8* r, s32 del);                 // 0x80213BCC
u8*   RoundSideInit(u8* r);                             // 0x80213B5C
void  BattleStartRound(u8* b);                          // 0x802124F4
void  BattleAdvance(u8* b);                             // 0x80212A00
void  BattleTakeChoice(u8* b);                          // 0x80213308
void  fn_802188E4(void* lab);
void  fn_802102DC(u8* b);
void  fn_80210634(u8* b);
}
extern u8 lbl_8038901C[];   // battle vtable
extern void* gLab;          // 0x805FBBC8

// Battle layout (gBattle):
//   +0x004 random generator (Mersenne twister, 624 words + index + seed)
//   +0xA0C round record (array-new of two 0x38 round sides)
//   +0xA1C listeners[count +0xA30], +0xA34 / +0xA38 owned objects (virtual dtor at vtable+8)
//   +0xA3C 2x2 block pools
//   +0xA60.. tuning copied from the game database

typedef void (*DestroyFn)(void* obj, s32 del);
static inline void VDestroy(void* obj) {
    (*(DestroyFn*)(*(u8**)obj + 8))(obj, 1);
}

extern "C" {

// 0x8032F2E0
// Runtime helper: destroy `count` array elements of `size` bytes, last first.
void __destroy_arr(void* arr, DestroyFn dtor, u32 size, u32 count) {
    u8* p = (u8*)arr + size * count;
    while (count != 0) {
        p -= size;
        dtor(p, -1);
        count--;
    }
}

// 0x80220A34
// Random generator not yet seeded (index 625) with the default seed.
void RngInit(u8* rng) {
    *(u32*)(rng + 0x9C0) = 625;
    *(u32*)(rng + 0x9C4) = 0xBEEF00;
}

// 0x80220A4C
void* RngDestroy(void* rng, s32 del) {
    if (rng != NULL && del > 0) operator_delete(rng);
    return rng;
}

// 0x80220688
// A pool of `count` blocks of `size` bytes each.
void* BlockPoolCreate(u8* pool, s32 count, u32 size) {
    *(u32*)pool = size;
    *(s32*)(pool + 4) = count;
    *(u32*)(pool + 8) = 0;
    void** blocks = (void**)MemAllocSmall(count * 4);
    *(void***)(pool + 0xC) = blocks;
    for (s32 i = 0; i < count; i++) (*(void***)(pool + 0xC))[i] = MemAllocSmall(size);
    return pool;
}

// 0x8022070C
void* BlockPoolDestroy(u8* pool, s32 del) {
    if (pool == NULL) return pool;
    for (s32 i = 0; i < *(s32*)(pool + 4); i++) MemFreeSmall((*(void***)(pool + 0xC))[i]);
    MemFreeSmall(*(void**)(pool + 0xC));
    if (del > 0) operator_delete(pool);
    return pool;
}

static void BattleDropObjects(u8* b) {
    for (s32 i = 0; i < *(s32*)(b + 0xA30); i++) {
        u8** slot = (u8**)(b + 0xA1C) + i;
        if (*slot != NULL) VDestroy(*slot);
        *slot = NULL;
    }
    *(s32*)(b + 0xA30) = 0;
    if (*(void**)(b + 0xA34) != NULL) VDestroy(*(void**)(b + 0xA34));
    *(void**)(b + 0xA34) = NULL;
    if (*(void**)(b + 0xA38) != NULL) VDestroy(*(void**)(b + 0xA38));
    *(void**)(b + 0xA38) = NULL;
}

static void RoundFree(u8* round) {
    if (round == NULL) return;
    __destroy_arr(round + 4, (DestroyFn)RoundSideDestroy, 0x38, 2);
    operator_delete(round);
}

// 0x80213660
// Back to an empty battle: listeners and owned objects dropped, no fighters, no round.
// (The original takes the tuning values as extra arguments but ignores them.)
void BattleClear(u8* b) {
    BattleDropObjects(b);
    *(s32*)(b + 0xA10) = 1;
    *(s32*)(b + 0xA18) = 0;
    *(s32*)(b + 0xA30) = 0;
    *(s32*)(b + 0xA4C) = 0;
    b[0xA50] = 0;
    b[0xA5C] = 0;
    for (s32 s = 0; s < 2; s++) {
        *(u32*)(b + 0xA00 + s * 4) = 0;
        b[0x9F0 + s] = 0;
        b[0x9F2 + s] = 0;
        b[0x9FC + s] = 0;
        *(u32*)(b + 0x9F4 + s * 4) = 0;
        b[0x9FE + s] = 0;
    }
    for (s32 i = 0; i < 4; i++) {
        *(u32*)(b + 0xA1C + i * 4) = 0;
        b[0xA2C + i] = 0;
    }
    *(u32*)(b + 0xA54) = 0;
    *(u32*)(b + 0xA58) = 0;
    RoundFree(*(u8**)(b + 0xA0C));
    *(u8**)(b + 0xA0C) = NULL;
    *(s32*)(b + 0x9CC) = -1;
    *(s32*)(b + 0x9D0) = -1;
    *(u32*)(b + 0x9D4) = 0;
    *(s32*)(b + 0x9D8) = -1;
    *(s32*)(b + 0x9DC) = -1;
    *(u32*)(b + 0x9E0) = 0;
    b[0xACD] = 0;
}

static inline f32 Tuning(u32 ofs) {
    return *(f32*)(*(u8**)(gGameDb->data + 0x99D0) + ofs);
}

// 0x8020F8E0
u8* BattleCtor(u8* b) {
    *(u8**)b = lbl_8038901C;
    RngInit(b + 4);
    *(u32*)(b + 0xA08) = 0;
    *(u32*)(b + 0xA0C) = 0;
    *(s32*)(b + 0xA10) = 1;
    *(f32*)(b + 0xA14) = 7.0f;
    *(u32*)(b + 0xA18) = 0;
    *(u32*)(b + 0xA30) = 0;
    *(u32*)(b + 0xA34) = 0;
    *(u32*)(b + 0xA38) = 0;
    *(u32*)(b + 0xA4C) = 0;
    b[0xA50] = 0;
    b[0xA5C] = 0;
    *(s32*)(b + 0xA60) = 10;
    *(s32*)(b + 0xA64) = 45;
    *(f32*)(b + 0xA68) = 1.0f;
    *(s32*)(b + 0xA6C) = 10;
    *(f32*)(b + 0xA70) = 0.2f;
    *(f32*)(b + 0xA74) = 0.05f;
    *(f32*)(b + 0xA78) = 0.1f;
    *(f32*)(b + 0xA7C) = 0.75f;
    *(f32*)(b + 0xA80) = 0.25f;
    *(s32*)(b + 0xA84) = 0;
    *(f32*)(b + 0xA88) = 0.0f;
    *(f32*)(b + 0xA8C) = 1.0f;
    *(f32*)(b + 0xA90) = 0.0f;
    *(s32*)(b + 0xA94) = 0;
    *(f32*)(b + 0xA98) = 0.0f;
    *(s32*)(b + 0xA9C) = 0;
    for (u32 o = 0xAA0; o <= 0xAB4; o += 4) *(f32*)(b + o) = 0.0f;
    for (u32 o = 0xABC; o <= 0xAC8; o += 4) *(s32*)(b + o) = 0;
    b[0xACC] = 0;
    b[0xACD] = 0;
    for (u32 o = 0xAEC; o <= 0xB00; o += 4) *(u32*)(b + o) = 0;
    BattleClear(b);

    for (s32 i = 0; i < 2; i++) {
        for (s32 j = 0; j < 2; j++) {
            u8* pool = (u8*)operator_new(0x10);
            if (pool != NULL) pool = (u8*)BlockPoolCreate(pool, 0x10, 0x40);
            *(u8**)(b + 0xA3C + i * 8 + j * 4) = pool;
        }
    }

    *(s32*)(b + 0xA60) = (s32)Tuning(0x54);
    *(s32*)(b + 0xA64) = (s32)Tuning(0x4C);
    *(f32*)(b + 0xA68) = Tuning(0x74);
    *(s32*)(b + 0xA6C) = (s32)Tuning(0xBC);
    *(f32*)(b + 0xA70) = Tuning(0xC4);
    *(f32*)(b + 0xA74) = Tuning(0x7C);
    *(f32*)(b + 0xA78) = Tuning(0x84);
    *(f32*)(b + 0xA7C) = Tuning(0x8C);
    *(f32*)(b + 0xA80) = Tuning(0x94);
    *(s32*)(b + 0xA84) = (s32)Tuning(0x1C4);
    *(f32*)(b + 0xA88) = Tuning(0x154);
    *(f32*)(b + 0xA8C) = Tuning(0x9C);
    *(f32*)(b + 0xA90) = Tuning(0xA4);
    *(s32*)(b + 0xA94) = (s32)Tuning(0x124);
    *(f32*)(b + 0xA98) = Tuning(0x16C);
    *(s32*)(b + 0xA9C) = (s32)Tuning(0x12C);
    *(f32*)(b + 0xAA0) = Tuning(0x174);
    *(f32*)(b + 0xAA4) = Tuning(0x15C);
    *(f32*)(b + 0xAA8) = Tuning(0x164);
    *(f32*)(b + 0xAAC) = Tuning(0x17C);
    *(f32*)(b + 0xAB0) = Tuning(0x184);
    *(s32*)(b + 0xABC) = (s32)Tuning(0x554);
    *(s32*)(b + 0xAC0) = (s32)Tuning(0x55C);
    *(s32*)(b + 0xAC4) = (s32)Tuning(0x564);
    *(s32*)(b + 0xAC8) = (s32)Tuning(0x56C);
    *(u32*)(b + 0xAD4) = 0;
    *(u32*)(b + 0xAD8) = 0;
    *(f32*)(b + 0xADC) = -1.0f;
    *(f32*)(b + 0xAE0) = -1.0f;
    *(u32*)(b + 0xAE4) = 0;
    *(u32*)(b + 0xAE8) = 0;
    return b;
}

// 0x8020FEEC
u8* BattleDtor(u8* b, s32 del) {
    if (b == NULL) return b;
    *(u8**)b = lbl_8038901C;
    BattleDropObjects(b);
    for (s32 i = 0; i < 2; i++)
        for (s32 j = 0; j < 2; j++) BlockPoolDestroy(*(u8**)(b + 0xA3C + i * 8 + j * 4), 1);
    RoundFree(*(u8**)(b + 0xA0C));
    RngDestroy(b + 4, -1);
    if (del > 0) operator_delete(b);
    return b;
}

typedef u32 (*TickFn)(void* obj, f32 dt);
typedef u32 (*PollFn)(void* obj);
static inline u32 VTick(void* o, f32 dt) { return (*(TickFn*)(*(u8**)o + 0xC))(o, dt); }
static inline u32 VCall(void* o, u32 slot) { return (*(PollFn*)(*(u8**)o + slot))(o); }

// 0x80210060
// Per-frame battle update: count the turn timer down and run the current phase.
void BattleUpdate(u8* b, f32 dt) {
    *(f32*)(b + 0xA14) -= dt;
    switch (*(u32*)(b + 0xA18)) {
    case 0:  // intro
        fn_802188E4(gLab);
        if (VTick(*(void**)(b + 0xA34), dt) == 1) BattleAdvance(b);
        break;
    case 1:
        fn_802102DC(b);
        break;
    case 2:  // choosing: take the choice, then tick the listeners still running
        BattleTakeChoice(b);
        // fallthrough
    case 5:
        for (s32 i = 0; i < *(s32*)(b + 0xA30); i++)
            if (b[0xA2C + i] == 0) VTick(((void**)(b + 0xA1C))[i], dt);
        break;
    case 3:
        fn_80210634(b);
        BattleAdvance(b);
        break;
    case 4: {  // new round record
        RoundFree(*(u8**)(b + 0xA0C));
        u8* r = (u8*)operator_new(0xD0);
        if (r != NULL) {
            *(s32*)r = 6;
            RoundSideInit(r + 4);
            RoundSideInit(r + 4 + 0x38);
            *(u8**)(r + 0x78) = b;
            *(u32*)(r + 0x7C) = 0;
            r[0x74] = 0;
            r[0x75] = 0;
            for (s32 i = 0; i < 10; i++) {
                *(s32*)(r + 0x80 + i * 4) = -1;
                *(s32*)(r + 0xA8 + i * 4) = -1;
            }
        }
        *(u8**)(b + 0xA0C) = r;
        BattleStartRound(b);
        break;
    }
    case 6:
    case 7:
        if (VCall(*(void**)(b + 0xA38), 0x14) == 1) BattleAdvance(b);
        break;
    case 8:
        if (VCall(*(void**)(b + 0xA34), 0x10) == 1 && *(u32*)(b + 0xA18) != 0) BattleAdvance(b);
        break;
    }
}

}  // extern "C"

