// Mixed small game functions from 0x8002B58C..0x800424DC: grab-hand setup, gem spawn, bonus
// spawn, catch minigame update and the battle intro state step.
#include <math.h>
#include <stdint.h>

#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*VFn2)(u8* obj, s32 a);                       // vtable slot: (obj, mask)
typedef s32 (*VFnHit)(u8* obj, s32 a, f32 x, f32 y);        // vtable slot 0x38 of a UI element
typedef u8* (*VFnPtr)(u8* obj);                             // vtable slot returning a pointer

extern "C++" u8* gBattle;           // 0x805FBBB8
extern "C++" void* gScreenSets;     // 0x805FB654
extern "C++" s32 gZoneResourceSize; // 0x805FB94C

extern "C" {
void BattleReset(u8* b);                                        // 0x80025AE4 (battle_core)
void CatchMinigame_UpdateBase(void* g, f32 dt);                 // 0x800224C4
u32 PadGetButtons(s32 chan);                                    // 0x801B9B40
s32 PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);    // 0x80209E70
u32 RandomU32(void);                                            // 0x80143460
void* ScreenSetFindElement(void* set, u32 hash);                // 0x80198F18
void* ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
u32 StringHash(const char* s);                                  // 0x801EB704
void fn_8003D7A8(void);                                         // 0x8003D7A8
void* fn_8003E39C(void);                                        // 0x8003E39C
s32 fn_8003F8D4(void);                                          // 0x8003F8D4
void fn_80036998(u8* a, u8* b, f32 dt);                         // 0x80036998
void fn_800383A0(u8* p);                                        // 0x800383A0
void fn_8004183C(void);                                         // 0x8004183C
void fn_80042408(u8* c, const s32* kind, s32 reaction, u8* fighter, s32 flag);  // 0x80042408
u8* fn_8004EDD0(u8* obj, s32 flag, s32 a2);                     // 0x8004EDD0
void fn_800E75E8(s32 a);                                        // 0x800E75E8
f32 fn_8018BE30(void* obj, f32 a);                              // 0x8018BE30
void* fn_80198F1C(void* set, u32 hash);                         // 0x80198F1C
s32 fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2);                  // 0x8019BBA4
void fn_801A1ABC(u8* el, u32 v);                                // 0x801A1ABC
void fn_801A8654(u8* obj, s32 a1);                              // 0x801A8654
void fn_801A8E80(u8* obj, s32 flag);                            // 0x801A8E80
u8* fn_801B3308(void);                                          // 0x801B3308
void fn_801B3310(u8* p, u8* q);                                 // 0x801B3310
void* fn_801D40E0(s32 n);                                       // 0x801D40E0
const char* fn_801EB708(u32 hash);                              // 0x801EB708
u8* fn_801F61AC(u8* arg0, s32 arg1);                            // 0x801F61AC
u8* fn_801F8110(u8* p, s32 a4);                                 // 0x801F8110
void fn_801F903C(u8* obj, f32* dir, f32 v);                     // 0x801F903C
void* fn_80209814(void* v);                                     // 0x80209814
void fn_8020A398(s32 handle);                                   // 0x8020A398

extern u8 gWorldState[];            // 0x803E6D00 (first word: world block pointer)
extern u8 lbl_803C4298[];           // 0x803C4298 (level info block; +0x1C u8 mode, +0x38 pad chan)
extern f32 lbl_8034756C[];          // 0x8034756C (gem kind weights)
extern f32 lbl_80347554[];          // 0x80347554
extern f32 lbl_80347560[];          // 0x80347560
extern s32 lbl_80347578[];          // 0x80347578
extern s32 lbl_80347584[];          // 0x80347584
extern s32 lbl_80347590[];          // 0x80347590
extern f32 lbl_8034759C[];          // 0x8034759C
extern f32 lbl_803475A8[];          // 0x803475A8
extern f32 lbl_803475B4[];          // 0x803475B4
extern f32 lbl_80347634[];          // 0x80347634
extern f32 lbl_80347648[];          // 0x80347648
extern f32 lbl_8034765C[];          // 0x8034765C
extern f32 lbl_80347670[];          // 0x80347670
extern s32 lbl_803C4DC0[];          // 0x803C4DC0 (per-kind texture cycle counters)
extern s32 lbl_805F8580;            // 0x805F8580
extern s32 lbl_805F8584;            // 0x805F8584
extern u8 lbl_805FB1C0;             // 0x805FB1C0
extern u8 lbl_805FB1C1;             // 0x805FB1C1
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
extern u8* lbl_805FBBE0;            // 0x805FBBE0 (save block pointer)
extern f32 lbl_805FCB40;            // 0x805FCB40 (0.0f)
extern f32 lbl_805FCB44;            // 0x805FCB44 (-1.0f)
extern f64 lbl_805FCB48;            // 0x805FCB48 (0.5)
extern f32 lbl_805FCB58;            // 0x805FCB58 (25.0f)
extern f64 lbl_805FCB60;            // 0x805FCB60 (0.6)
extern f32 lbl_805FCD88;            // 0x805FCD88 (0.0f)
extern f32 lbl_805FCD8C;            // 0x805FCD8C (1.0f)
extern f32 lbl_805FCDA4;            // 0x805FCDA4 (2.3283064e-10f, 1/2^32)
extern f32 lbl_805FCDBC;            // 0x805FCDBC (0.0f)
extern f32 lbl_805FCDE0;            // 0x805FCDE0 (0.5f)
extern f32 lbl_805FCEBC;            // 0x805FCEBC (0.0f)
extern f32 lbl_805FD150;            // 0x805FD150 (0.0f)
extern f32 lbl_805FD160;            // 0x805FD160 (threshold)
}

// Reciprocal square root, refined like the frsqrte + two Newton steps in the original.
static f32 RSqrtRefined(f32 x) { return (f32)(1.0 / sqrt((f64)x)); }

// Lazily creates the voice SFX handle (lbl_805FBBAC) the first time a sound is played.
static inline void EnsureVoiceHandle() {
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) h = fn_80209814(h);
        lbl_805FBBAC = h;
    }
}

// Releases the sound handle stored at p (if any) and marks the slot as free (-1).
static inline void ReleaseHandle(s32* p) {
    if (*p != -1) {
        fn_8020A398(*p);
        *p = -1;
    }
}

extern "C" {

// 0x8002B58C
// Sets up the grab-hand object: bounds from the two rectangles, grab cone/shadow sub-objects
// facing down (0,-1,0), the "hand" screen element and the matching zone entry.
u8* fn_8002B58C(u8* arg0, void* arg1, s32 arg2, u8* arg3, u8* arg4, u8* arg5) {
    f32 dir[3];
    u8* set;
    s32 zoneIdx;
    s32 i;

    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0x14) = arg2;
    *(f32*)(arg0 + 0x18) = lbl_805FCB40;
    *(s32*)(arg0 + 0x1C) = *(s32*)(arg3 + 0x0);
    *(s32*)(arg0 + 0x20) = *(s32*)(arg3 + 0x4);
    *(s32*)(arg0 + 0x24) = *(s32*)(arg4 + 0x0);
    *(s32*)(arg0 + 0x28) = *(s32*)(arg4 + 0x4);
    *(s32*)(arg0 + 0x2C) = 0;
    *(s32*)(arg0 + 0x48) = 0;
    *(s32*)(arg0 + 0x4C) = 2;
    fn_801F8110(arg0 + 0x54, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F61AC(arg0 + 0x4D8, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(arg0 + 0x594) = 0;
    *(s32*)(arg0 + 0x598) = 0;
    *(s32*)(arg0 + 0x59C) = 0;
    *(s32*)(arg0 + 0x5A0) = 0;
    *(s32*)(arg0 + 0x5A4) = 0;
    *(s32*)(arg0 + 0x5A8) = 0;
    *(s32*)(arg0 + 0x5AC) = 0;
    *(s32*)(arg0 + 0x5B0) = 0;
    *(s32*)(arg0 + 0x5B4) = 0;
    *(f32*)(arg0 + 0x5B8) = lbl_805FCB40;
    fn_801B3310(fn_801B3308(), arg0 + 0x4D8);

    // Direction (0,-1,0) built from integer constants, then normalised.
    f32 vx = (f32)0;
    f32 vy = (f32)-1;
    f32 vz = (f32)0;
    *(s32*)(arg0 + 0xEC) = 1;
    *(f32*)(arg0 + 0xF0) = lbl_805FCB40;
    *(f32*)(arg0 + 0xF4) = lbl_805FCB44;
    *(f32*)(arg0 + 0xF8) = lbl_805FCB40;
    f32 len2 = vz * vz + (vx * vx + vy * vy);
    if (lbl_805FCB40 == len2) {
        dir[0] = lbl_805FCB40;
        dir[1] = lbl_805FCB40;
        dir[2] = lbl_805FCB40;
    } else {
        f32 r = RSqrtRefined(len2);
        dir[0] = vx * r;
        dir[1] = vy * r;
        dir[2] = vz * r;
    }
    fn_801F903C(arg0 + 0x54, dir, lbl_805FCB58);
    fn_801B3310(fn_801B3308(), arg0 + 0x54);

    set = (u8*)ScreenSetsCurrent(gScreenSets);
    *(void**)(arg0 + 0x48) = ScreenSetFindElement(set, StringHash("hand"));
    *(u8**)(arg0 + 0x2C) = arg5;
    *(s32*)(arg0 + 0xC) = 5;
    *(void**)(arg0 + 0x0) = arg1;

    // Grow the hit rectangle by 0.6 * 0.5 * (width at +0x74 of the owner).
    f32 grow = (f32)((f64)lbl_805FCB60 * (f64)*(f32*)((u8*)arg1 + 0x74) * (f64)lbl_805FCB48);
    f32 l = *(f32*)(arg0 + 0x1C);
    f32 t = *(f32*)(arg0 + 0x20);
    f32 rr = *(f32*)(arg0 + 0x24);
    f32 b = *(f32*)(arg0 + 0x28);
    *(f32*)(arg0 + 0x18) = grow;
    *(f32*)(arg0 + 0x1C) = l + grow;
    *(f32*)(arg0 + 0x20) = t + grow;
    *(f32*)(arg0 + 0x24) = rr - grow;
    *(f32*)(arg0 + 0x28) = b - grow;

    // Find the zone object whose type hash is the "hand" marker and remember it at +0x50.
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + (gZoneResourceSize * 2 - gZoneResourceSize);
    for (zoneIdx = 0; zoneIdx < *(s32*)(*(u8**)gWorldState + 0x4); zoneIdx++) {
        for (i = 0; i < *(s32*)(zone + 0x98); i++) {
            u8* obj = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
            u8* info = *(u8**)(obj + 0x9C);
            u32 h = (info != NULL) ? *(u32*)(info + 0x8) : 0;
            fn_801EB708(h);
            info = *(u8**)(obj + 0x9C);
            h = (info != NULL) ? *(u32*)(info + 0x8) : 0;
            if (h == 0xD716DA31u) {
                *(u8**)(arg0 + 0x50) = obj;
            }
        }
    }
    *(void**)(arg0 + 0x594) = ScreenSetFindElement(set, StringHash("InvisibleHand"));
    return arg0;
}

}  // extern "C"

// Picks the texture variant of one gem kind and initialises the gem record: x position,
// colour, size, animation and the screen element (cycling through three element hashes).
static void GemInit(u8* arg0, void* set, s32 idx, s32 cursor, const char* animName, u32 hashA, u32 hashB,
                    u32 hashC) {
    f32 lo = (f32)lbl_80347584[idx];
    f32 hi = (f32)lbl_80347590[idx];
    f32 a = lbl_80347554[idx];
    f32 b = lbl_80347560[idx];
    *(s32*)(arg0 + 0x0) = (s32)(lo + (hi - lo) * (lbl_805FCDA4 * (f32)RandomU32()));
    f32 t = lbl_805FCDA4 * (f32)RandomU32();
    *(f32*)(arg0 + 0x30) = lbl_8034759C[idx];
    *(f32*)(arg0 + 0x34) = lbl_803475A8[idx];
    *(f32*)(arg0 + 0x38) = lbl_803475B4[idx];
    *(s32*)(arg0 + 0x10) = lbl_80347578[idx];
    *(f32*)(arg0 + 0x28) = a + (b - a) * t;
    *(void**)(arg0 + 0x20) = fn_80198F1C(set, StringHash(animName));
    switch (lbl_803C4DC0[cursor]) {
    case 0:
        *(void**)(arg0 + 0x54) = ScreenSetFindElement(set, hashA);
        break;
    case 1:
        *(void**)(arg0 + 0x54) = ScreenSetFindElement(set, hashB);
        break;
    case 2:
        *(void**)(arg0 + 0x54) = ScreenSetFindElement(set, hashC);
        break;
    }
    lbl_803C4DC0[cursor] = (lbl_803C4DC0[cursor] + 1) % 3;
}

extern "C" {

// 0x800339A8
// Spawns a gem: rolls its kind from the weights table (0, 1 or 2), initialises the record for that
// kind and sets the gem / shatter textures on the two attached images. Returns the kind.
s32 fn_800339A8(u8* arg0) {
    s32 kind;
    f32 roll = lbl_805FCD88 + lbl_805FCD8C * (lbl_805FCDA4 * (f32)RandomU32());
    if (roll < lbl_8034756C[2]) {
        kind = 0;
    } else if (roll < lbl_8034756C[2] + lbl_8034756C[1]) {
        kind = 1;
    } else {
        kind = 2;
    }
    void* set = ScreenSetsCurrent(gScreenSets);
    switch (kind) {
    case 0:
        GemInit(arg0, set, 2, 0, "MiningSpray_BlueAnimation", 0xC8E2F6DAu, 0xBFE5C64Cu, 0x26EC97F6u);
        break;
    case 1:
        GemInit(arg0, set, 1, 1, "MiningSpray_RedAnimation", 0x94531DEEu, 0xE3542D78u, 0x7A5D7CC2u);
        break;
    case 2:
        GemInit(arg0, set, 0, 2, "MiningSpray_GreenAnimation", 0x1342433Au, 0x644573ACu, 0xFD4C2216u);
        break;
    }
    if (*(u8**)(arg0 + 0x18) != NULL && *(u8**)(arg0 + 0x1C) != NULL) {
        u8* o = *(u8**)(arg0 + 0x18);
        UIImage_SetTexture((u32)(uintptr_t) * (void**)o,  // lint:ok header takes u32
                           (BOOL)UITextureList_GetTexture(StringHash("GemTextureList"), kind));
        o = *(u8**)(arg0 + 0x18);
        (*(VFn2*)(*(u8**)(o + 0x138) + 0x30))(o, 2);
        o = *(u8**)(arg0 + 0x1C);
        UIImage_SetTexture((u32)(uintptr_t) * (void**)o,  // lint:ok header takes u32
                           (BOOL)UITextureList_GetTexture(StringHash("ShatterTextureList"), kind));
        o = *(u8**)(arg0 + 0x1C);
        (*(VFn2*)(*(u8**)(o + 0x138) + 0x30))(o, 2);
    }
    return kind;
}

// 0x80035A04
// With a chance of n percent spawns a falling bonus item of one of four kinds (weighted by the free
// slots left in each kind's table), places it at a random x above the screen and registers it.
// Returns 1 if an item was spawned.
s32 fn_80035A04(u8* arg0, s32 arg1) {
    f32 scale = *(f32*)(arg0 + 0x5EC);
    u32 rnd = RandomU32();
    if (rnd % 100 >= (u32)arg1) return 0;

    u32 xPos = rnd % 0x172 + 0x8C;
    s32 range = (s32)*(f32*)(arg0 + 0x178);
    s32 yPos = -0x20 - (s32)(RandomU32() % (u32)range);
    s32 s1 = 10 - *(s32*)(arg0 + 0x250);
    s32 s2 = (s1 + 10) - *(s32*)(arg0 + 0x360);
    s32 s3 = (s2 + 10) - *(s32*)(arg0 + 0x420);
    s32 total = (s3 + 10) - *(s32*)(arg0 + 0x4E0);
    if (total <= 0) return 0;

    s32 pick = (s32)(rnd % (u32)total);
    s32 kind;
    ScriptValue tex;
    if (pick < s1) {
        kind = 1;
        tex = UITextureList_GetTexture(0xACCED4C1u, 0);
    } else if (pick < s2) {
        kind = 2;
        tex = UITextureList_GetTexture(0xD2D850FCu, 0);
    } else if (pick < s3) {
        kind = 4;
        tex = UITextureList_GetTexture(0x3095961Fu, 0);
    } else if (pick < total) {
        kind = 8;
        tex = UITextureList_GetTexture(0x2EFDE767u, 0);
    } else {
        return 0;
    }

    EnsureVoiceHandle();
    PlaySound(0xB30C11C0u, 0, 0, 0, 0, 0);
    EnsureVoiceHandle();
    PlaySound(0xCA63AAC7u, 0, 0, 0, 0, 0);

    u8* slots;   // element pointers (stride 4)
    u8* count;   // number of live items of this kind
    u8* used;    // per-slot in-use flags
    u8* vel;     // per-slot vec3 (stride 0xC)
    u8* state;   // per-slot state bytes
    switch (kind) {
    case 1:
        slots = arg0 + 0x198;
        count = arg0 + 0x250;
        used = arg0 + 0x1C0;
        vel = arg0 + 0x1CC;
        state = arg0 + 0x244;
        break;
    case 2:
        slots = arg0 + 0x2A8;
        count = arg0 + 0x360;
        used = arg0 + 0x2D0;
        vel = arg0 + 0x2DC;
        state = arg0 + 0x354;
        break;
    case 4:
        slots = arg0 + 0x368;
        count = arg0 + 0x420;
        used = arg0 + 0x390;
        vel = arg0 + 0x39C;
        state = arg0 + 0x414;
        break;
    default:
        slots = arg0 + 0x428;
        count = arg0 + 0x4E0;
        used = arg0 + 0x450;
        vel = arg0 + 0x45C;
        scale = *(f32*)(arg0 + 0x5F0) - lbl_805FCDE0;
        state = arg0 + 0x4D4;
        break;
    }
    if (*(s32*)count >= 10) return 0;

    for (s32 i = 0; i < 10; i++) {
        if (used[i] != 0) continue;
        u8* el = *(u8**)(slots + i * 4);
        UIImage_SetTexture((u32)(uintptr_t) * (void**)el, (BOOL)tex);  // lint:ok header takes u32
        el = *(u8**)(slots + i * 4);
        (*(VFn2*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x1A);
        el = *(u8**)(slots + i * 4);
        *(f32*)(el + 0x70) = scale;
        *(f32*)(vel + i * 0xC) = lbl_805FCDBC;
        *(f32*)(vel + i * 0xC + 4) = lbl_805FCDBC;
        *(f32*)(vel + i * 0xC + 8) = lbl_805FCDBC;
        el = *(u8**)(slots + i * 4);
        *(f32*)(el + 0x68) = (f32)(s32)xPos;
        *(f32*)(el + 0x6C) = (f32)yPos;
        el = *(u8**)(slots + i * 4);
        UIElement_SetPos((u32)(uintptr_t) * (void**)el, (f32*)(el + 0x68));  // lint:ok header takes u32
        *(s32*)count = *(s32*)count + 1;
        used[i] = 1;
        state[i] = 0;
        return 1;
    }
    return 0;
}

// 0x80037850
// Catch-minigame per-frame update: grab/release input (buttons 1 and 2) with the hand sound,
// hand scale and position tracking, success/fail timers and the exit when the level scrolls out.
void fn_80037850(u8* arg0, f32 farg0) {
    CatchMinigame_UpdateBase(arg0, farg0);
    if (*(s32*)(arg0 + 0x14C) != 0 && *(u8**)(arg0 + 0x154) != NULL) {
        fn_801A8654(*(u8**)(arg0 + 0x154), 1);
        u8* hand = *(u8**)(arg0 + 0x154);
        (*(VFn2*)(*(u8**)(hand + 0x138) + 0x2C))(hand, 2);
    }
    if (*(s32*)(arg0 + 0x14C) != 0) {
        s32 released = 0;
        if (WasButtonReleased(1) != 0 || WasButtonReleased(2) != 0) released = 1;
        s32 holding = (released == 0);
        *(s32*)(arg0 + 0x14C) = holding;
        if (!holding) {
            ReleaseHandle((s32*)(arg0 + 0xA5C));
            EnsureVoiceHandle();
            PlaySound(0x742575F9u, 0, 0, 0, 0, 0);
        }
    }
    if (*(s32*)(arg0 + 0x14C) == 0) {
        s32 pressed = 0;
        if (WasButtonPressed(1) != 0 || WasButtonPressed(2) != 0) pressed = 1;
        *(s32*)(arg0 + 0x14C) = pressed;
        if (pressed != 0) {
            EnsureVoiceHandle();
            PlaySound(0x96DF0B06u, 0, 0, 0, 0, 0);
        }
    }
    if (*(s32*)(arg0 + 0x14C) != 0) {
        *(s32*)(arg0 + 0x9B0) = 1;
        *(f32*)(arg0 + 0x9AC) = lbl_805FCEBC;
        fn_80036998(arg0 + 0x170, arg0, farg0);
        f32 progress = *(f32*)(arg0 + 0x988);
        if (progress > lbl_805FCEBC) {
            f32 ratio = *(f32*)(arg0 + 0x98C) / progress;
            u8* fx = *(u8**)(arg0 + 0xA64);
            *(f32*)(fx + 0x74) = *(f32*)(arg0 + 0xA68) * (ratio / lbl_80347670[*(s32*)(arg0 + 0x164)]);
            f32 px = *(f32*)(arg0 + 0xA6C);
            f32 py = *(f32*)(arg0 + 0xA70);
            f32 pz = *(f32*)(arg0 + 0xA74);
            fx = *(u8**)(arg0 + 0xA64);
            *(f32*)(fx + 0x68) = px;
            *(f32*)(fx + 0x6C) = py;
            *(f32*)(fx + 0x70) = pz;
            fx = *(u8**)(arg0 + 0xA64);
            (*(VFn2*)(*(u8**)(fx + 0x138) + 0x2C))(fx, 0x1A);
        }
        if (*(u8**)(arg0 + 0x154) != NULL) {
            fn_801A1ABC(*(u8**)(arg0 + 0x980), 0);
            u8* hit = *(u8**)(arg0 + 0x980);
            u8* target = *(u8**)(arg0 + 0x158);
            if ((*(VFnHit*)(*(u8**)(hit + 0x138) + 0x38))(hit, 0, *(f32*)(target + 0x68),
                                                            *(f32*)(target + 0x6C)) != 0) {
                *(f32*)(*(u8**)(arg0 + 0x154) + 0x68) = *(f32*)(*(u8**)(arg0 + 0x158) + 0x68);
                *(f32*)(*(u8**)(arg0 + 0x154) + 0x6C) = *(f32*)(*(u8**)(arg0 + 0x158) + 0x6C);
            } else {
                fn_801A8654(*(u8**)(arg0 + 0x154), 0);
            }
            fn_801A1ABC(*(u8**)(arg0 + 0x980), 1);
            u8* hand = *(u8**)(arg0 + 0x154);
            (*(VFn2*)(*(u8**)(hand + 0x138) + 0x2C))(hand, 0x18);
        }
    } else {
        if (*(s32*)(arg0 + 0x9B0) != 0) {
            *(f32*)(arg0 + 0x9AC) = *(f32*)(arg0 + 0x9AC) + farg0;
        }
        fn_801A8654(*(u8**)(arg0 + 0x154), 0);
        f32 base = *(f32*)(arg0 + 0x170);
        *(f32*)(arg0 + 0x974) = base;
        *(f32*)(arg0 + 0x978) = base;
        *(f32*)(arg0 + 0x97C) = lbl_805FCEBC;
    }
    if (*(s32*)(arg0 + 0x984) != 0) {
        s32 score = *(s32*)(arg0 + 0x16C);
        *(s32*)(arg0 + 0xA54) = 1;
        *(s32*)(arg0 + 0x16C) = score + 1;
        fn_8019BBA4(*(u8**)(arg0 + 0xA44), *(u8**)(arg0 + 0xA4C), 0xA);
        u8* el = *(u8**)(arg0 + 0xA44);
        (*(VFn2*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
        PlaySound(0xD813918Fu, 0, 0, 0, 0, 0);
        fn_800383A0(arg0);
    }
    s32 failed = 0;
    if (*(s32*)(arg0 + 0x9B0) != 0 && *(f32*)(arg0 + 0x9AC) > lbl_80347634[*(s32*)(arg0 + 0x164)]) {
        failed = 1;
    }
    if (failed == 0) {
        if (*(s32*)(arg0 + 0x998) != 0 && *(f32*)(arg0 + 0x994) > lbl_80347648[*(s32*)(arg0 + 0x990)]) {
            failed = 1;
        }
        if (failed == 0) {
            if (*(s32*)(arg0 + 0x9A0) != 0 &&
                *(f32*)(arg0 + 0x99C) > lbl_8034765C[*(s32*)(arg0 + 0x990)]) {
                failed = 1;
            }
        }
    }
    if (failed != 0) {
        *(s32*)(arg0 + 0xA58) = 1;
        fn_8019BBA4(*(u8**)(arg0 + 0xA48), *(u8**)(arg0 + 0xA4C), 0xA);
        u8* el = *(u8**)(arg0 + 0xA48);
        (*(VFn2*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
        PlaySound(0x1E4A1327u, 0, 0, 0, 0, 0);
        fn_800383A0(arg0);
        ReleaseHandle((s32*)(arg0 + 0xA5C));
        ReleaseHandle((s32*)(arg0 + 0xA60));
    }
    u8* el = *(u8**)(arg0 + 0xA48);
    if (*(s32*)(el + 0x10) == 0) {
        if (*(s32*)(arg0 + 0xA58) != 0) {
            *(s32*)(arg0 + 0xA58) = 0;
            fn_8019BBA4(el, *(u8**)(arg0 + 0xA50), 0xA);
        } else {
            (*(VFn2*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        }
    }
    el = *(u8**)(arg0 + 0xA44);
    if (*(s32*)(el + 0x10) == 0) {
        if (*(s32*)(arg0 + 0xA54) != 0) {
            *(s32*)(arg0 + 0xA54) = 0;
            fn_8019BBA4(el, *(u8**)(arg0 + 0xA50), 0xA);
        } else {
            (*(VFn2*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        }
    }
    if (*(f32*)(arg0 + 0x6C) < lbl_805FCEBC) {
        GameFlowSignal(0x117741C3, 0, 0);
        StopRumble(*(s32*)(lbl_803C4298 + 0x38));
        ReleaseHandle((s32*)(arg0 + 0xA60));
        ReleaseHandle((s32*)(arg0 + 0xA5C));
        if (*(s32*)(arg0 + 0x14C) != 0) {
            EnsureVoiceHandle();
            PlaySound(0x742575F9u, 0, 0, 0, 0, 0);
        }
        fn_801A8E80(*(u8**)(arg0 + 0x154), 0);
        u8* hand = *(u8**)(arg0 + 0x154);
        (*(VFn2*)(*(u8**)(hand + 0x138) + 0x30))(hand, 2);
    }
}

// 0x800424DC
// Battle intro state step. State at +0x10: 0 = first-fighter intro (sound / voice cue), 1 = second
// fighter reaction setup, 2 = idle, otherwise finish: pick the result flag for the save block,
// signal the game flow and call the owner's virtual slot 0x18. Returns 1 when done.
s32 fn_800424DC(u8* arg0) {
    if (lbl_803C4298[0x1C] == 0x1D) {
        if (PadGetButtons(*(s32*)(lbl_803C4298 + 0x38)) != 0) {
            *(f32*)(arg0 + 0x4) = lbl_805FD150;
            fn_8004183C();
            BattleReset(gBattle);
            return 1;
        }
        return 0;
    }
    if (arg0[0xD] != 0) {
        *(s32*)(arg0 + 0x10) = 3;
    }
    u8* fighterB = *(u8**)(gBattle + 0xA4C);
    u8* other = *(u8**)(gBattle + 0xA00);
    if (other == fighterB) {
        other = *(u8**)(gBattle + 0xA04);
    }
    u8* anim = *(u8**)(arg0 + 0x14);
    u8* fighterA = *(u8**)(gBattle + 0xA00);
    bool advanced = false;
    if (anim != NULL) {
        if (fn_8018BE30(anim, lbl_805FD150) >= lbl_805FD160) {
            *(s32*)(arg0 + 0x10) = *(s32*)(arg0 + 0x10) + 1;
            advanced = true;
        }
    }
    if (!advanced) {
        if (arg0[0x9] != 0 && *(s32*)(arg0 + 0x10) < 3) {
            return 0;
        }
    }
    s32 state = *(s32*)(arg0 + 0x10);
    if (state >= 0 && state < 3) {
        if (state == 0) {
            u8* inner = *(u8**)(other + 0x10);
            u8* subject = (inner != NULL) ? *(u8**)(inner + 0x20) : NULL;
            if (subject != NULL) subject -= 0x264;
            if (gBattle[0xA5C] != 0) {
                fn_8004EDD0(subject, 0, 0);
                PlaySound(0xA34660C4u, 0, 0, 0, 0, 0);
            } else if (other == fighterA) {
                u8* hud = *(u8**)(arg0 + 0x1C);
                (*(VFn2*)(*(u8**)hud + 0x84))(hud, 0x0B8025EF);
                PlaySound(0xA34660C4u, 0, 0, 0, 0, 0);
            }
            arg0[0x9] = 1;
            *(s32*)(arg0 + 0x10) = *(s32*)(arg0 + 0x10) + 1;
            state = 1;
        }
        if (state == 1) {
            u8* inner = *(u8**)(fighterB + 0x10);
            u8* subject = (inner != NULL) ? *(u8**)(inner + 0x20) : NULL;
            if (subject != NULL) subject -= 0x264;
            *(u8**)(arg0 + 0x14) = (*(VFnPtr*)(*(u8**)subject + 0x10))(subject);
            if (fn_8003F8D4() != 0 && fighterB == *(u8**)(gBattle + 0xA04)) {
                fn_80042408(arg0, &lbl_805F8580, 0x24, fighterB, 0);
            } else {
                lbl_805FB1C1 = 0;
                fn_80042408(arg0, &lbl_805F8584, 0x34, fighterB, 0);
            }
            lbl_805FB1C0 = (fighterB != *(u8**)(gBattle + 0xA00));
            if (fighterB == fighterA) {
                PlaySound(0xA71D3E8Eu, 0, 0, 0, 0, 0);
            }
            *(f32*)(arg0 + 0x4) = lbl_805FD150;
        }
        return 0;
    }
    arg0[0x8] = (fighterB == *(u8**)(gBattle + 0xA00));
    u8 final = gBattle[0xA5C];
    if (fn_8003E39C() != NULL) {
        fn_8003E39C();
        fn_8003D7A8();
    }
    if (arg0[0x8] != 0) {
        *(s32*)(lbl_805FBBE0 + 0x138) = (final == 0) ? 0 : 3;
    } else {
        *(s32*)(lbl_805FBBE0 + 0x138) = (final == 0) ? 1 : 2;
    }
    fn_800E75E8(1);
    GameFlowSignal(0xBB234A92, 0, 0);
    (*(VFn2*)(*(u8**)arg0 + 0x18))(arg0, 0);
    return 1;
}

}  // extern "C"
