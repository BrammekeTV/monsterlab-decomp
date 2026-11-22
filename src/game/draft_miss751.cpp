// Misc object functions: bug-catching minigame setup, object reset/activate, active-character
// list registration, and locator effect update.
#include <stdint.h>

#include "types.h"

extern "C++" s32 gZoneResourceSize;   // 0x805FB94C
extern "C++" u8* fn_801B55A8(u8* ent);   // 0x801B55A8 (C++ linkage; returns the locator count as an integer value)

extern "C" {

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

s32  PlaySoundAt(void* sound, int a, const void* pos, int b);      // 0x80209ABC
u32  RandomU32(void);                                              // 0x80143460
void ScopedLockCtor(ScopedLock* l);                                // 0x801F5384
void ScopedLockDtor(ScopedLock* l, s32 flags);                     // 0x801F53B0
s32  SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);  // 0x801C80E0
u32  StringHash(const char* s);                                    // 0x801EB704
int  SysSprintf(char* buf, const char* fmt, ...);                  // 0x80142EF8
s32  TriggerDoAction(void* action, void* a1, void* a2);            // 0x801E0054
s32  fn_8004F0DC(void* slot, const char* name, s32 loop);          // 0x8004F0DC
void fn_80050164(u8* a0, u8* ctx);                                 // 0x80050164
s32  fn_80062EA8(u8* obj, s32 a);                                  // 0x80062EA8
s32  fn_80069180(u8* p, s32 a1, s32 a2);                           // 0x80069180 (returns fn_80062DC4's result, see fn_801B5BD8: 1)
void fn_8014474C(u8* src, f32* out);                               // 0x8014474C
void fn_801B55F4(u8* obj, u8* out, s32 idx);                       // 0x801B55F4
s32  fn_801DA780(u8* a3);                                          // 0x801DA780
u8*  fn_801DA788(u8* a3, s32 i);                                   // 0x801DA788

extern u8  gGame[];            // 0x803B53A0
extern u8  gWorldState[];      // 0x803E6D00 (first word: world block pointer)
extern u8  lbl_8047C760[];     // 0x8047C760 (effect manager)

typedef s32 (*VFn_i)(u8*);
typedef s32 (*VFn_ii)(u8*, s32);
typedef u8* (*VFn_pf)(u8*, u8*, f32);
typedef void (*VFn_pfff)(u8*, f32*, f32*, f32*);

// 0x800294BC
// Bug-catching setup: stores the owner/target pointers, derives two radii from the target's
// size (max of +0x74/+0x78 scaled by 0.6/0.7 and halved), offsets the second object's +0x70,
// randomizes a speed factor, then finds the "bug_catchingA" object in the current zone.
void fn_800294BC(u8* arg0, u8* arg1, u8* arg2, u8* arg3, s32 arg4) {
    *(u8**)(arg0 + 0x114) = arg2;
    *(u8**)(arg0 + 0x118) = arg3;
    *(s32*)(arg0 + 0x8) = arg4;
    *(u8**)(arg0 + 0x104) = arg1;

    f32 a = *(f32*)(arg1 + 0x74);
    f32 b = *(f32*)(arg1 + 0x78);
    f32 m = (a > b) ? a : b;
    *(f32*)(arg0 + 0x34) = (f32)(0.6 * (f64)m * 0.5);

    u8* o2 = *(u8**)(arg0 + 0x114);
    f32 c = *(f32*)(o2 + 0x74);
    f32 d = *(f32*)(o2 + 0x78);
    f32 m2 = (c > d) ? c : d;
    *(f32*)(arg0 + 0x38) = (f32)(0.7 * (f64)m2 * 0.5);

    *(f32*)(*(u8**)(arg0 + 0x114) + 0x70) = (f32)(0.1 + (f64)*(f32*)(*(u8**)(arg0 + 0x104) + 0x70));

    f32 r = 2.3283064e-10f * (f32)RandomU32();
    *(f32*)(arg0 + 0xC) = *(f32*)(arg0 + 0x34) * (1.0f + (1.0f + ((5.0f - 1.0f) * r)));

    u32 hash = StringHash("bug_catchingA");
    u8* slot = *(u8**)(*(u8**)gWorldState + 0x1A4) + ((gZoneResourceSize * 2) - gZoneResourceSize);
    for (s32 i = 0; i < *(s32*)(*(u8**)gWorldState + 0x4); i++) {
        for (s32 j = 0; j < *(s32*)(slot + 0x98); j++) {
            u8* ent = *(u8**)(*(u8**)(slot + 0x12C) + j * 4);
            u8* info = *(u8**)(ent + 0x9C);
            u32 h = (info != NULL) ? *(u32*)(info + 0x8) : 0;
            if (hash == h) *(u8**)(arg0 + 0x120) = ent;
        }
    }
}

// 0x8004EA70
// Activates/resets an object: runs the base reset (fn_80069180), then either starts the idle
// animation (when virtual 0xCC says so) or runs virtual 0x18 under a scoped lock, clears the
// motion vectors, and re-arms every type-1 entry of the sub-lists with fn_80050164.
s32 fn_8004EA70(u8* arg0, s32 arg1, s32 arg2) {
    ScopedLock lock;
    f32 pos[3];

    s32 res = fn_80069180(arg0, arg1, arg2);
    if ((*(VFn_i*)(*(u8**)arg0 + 0xCC))(arg0) != 0) {
        if (*(u8*)(arg0 + 0x28C) != 0) {
            fn_8004F0DC(arg0, (const char*)(arg0 + 0x28D), *(u8*)(arg0 + 0x38D));
        }
    } else {
        ScopedLockCtor(&lock);
        (*(VFn_ii*)(*(u8**)arg0 + 0x18))(arg0, 0);
        ScopedLockDtor(&lock, -1);
        *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x80;
    }
    if (res != 0) *(u32*)(arg0 + 0x15C) |= 0x200000;

    *(f32*)(arg0 + 0x26C) = 0.0f;
    u8* st = *(u8**)(arg0 + 4);
    *(f32*)(arg0 + 0x270) = 0.0f;
    *(f32*)(arg0 + 0x274) = 0.0f;
    *(f32*)(arg0 + 0x280) = 0.0f;
    *(f32*)(arg0 + 0x278) = 0.0f;
    *(f32*)(arg0 + 0x27C) = 0.0f;
    *(f32*)(arg0 + 0x284) = 1.0f;
    *(f32*)(arg0 + 0x288) = 1.0f;
    pos[2] = 0.0f;
    *(u32*)(st + 0x20) &= ~1u;
    pos[1] = 0.0f;
    pos[0] = 0.0f;

    // f1 (0.0f) is a leftover register here; the callee's third argument is not set by the caller.
    u8* lst = (*(VFn_pf*)(*(u8**)arg0 + 0xC))(arg0, st, 0.0f);
    for (s32 i = 0; i < fn_801DA780(lst); i++) {
        u8* grp = *(u8**)(fn_801DA788(lst, i) + 0x10);
        for (s32 j = 0; j < *(s32*)(grp + 0x6C); j++) {
            u8* e = *(u8**)(grp + 0x70 + j * 8);
            if (e != NULL && *(u8*)(e + 0xC) == 1) {
                fn_80050164(arg0, e);
            }
        }
    }
    return res;
}

// 0x800535F8
// Registers an object in the 32-slot active-character list at gGame+0x140 (once), spawns its
// def effect + sound at its position when arg1 == 0, and fires the def's and the object's
// trigger actions if set.
void fn_800535F8(u8* arg0, s32 arg1) {
    char buf[0x100];
    f32 pos[4];

    fn_80062EA8(arg0, arg1);

    u8 cnt = gGame[0x13C];
    u8 i;
    bool found = false;
    for (i = 0; i < cnt; i++) {
        if (*(u8**)(gGame + 0x140 + i * 4) == arg0) { found = true; break; }
    }
    if (!found) {
        for (i = 0; i < cnt; i++) {
            if (*(u8**)(gGame + 0x140 + i * 4) == NULL) {
                *(u8**)(gGame + 0x140 + i * 4) = arg0;
                found = true;
                break;
            }
        }
        if (!found) {
            if (cnt < 0x20) {
                u8 n = gGame[0x13C];
                *(u8**)(gGame + 0x140 + n * 4) = arg0;
                gGame[0x13C] = (u8)(n + 1);
            } else {
                // trailing varargs (count, float) are stale registers
                SysSprintf(buf, "Holy toledo, we have more than %d active characters in this buffer.", 0x20);
            }
        }
    }

    if (arg1 == 0) {
        u8* def = *(u8**)(arg0 + 0x9C);
        if (def != NULL) {
            u32 fx = *(u32*)(def + 0x1E0);
            if (fx != 0) {
                u8* p = *(u8**)(arg0 + 0x4);
                pos[0] = *(f32*)(p + 0x0);
                pos[1] = *(f32*)(p + 0x4);
                pos[2] = *(f32*)(p + 0x8);
                pos[3] = 0.0f;
                SpawnEffectAt(lbl_8047C760, fx, pos, NULL);
                PlaySoundAt(*(void**)(*(u8**)(arg0 + 0x9C) + 0x1F0), 0, *(u8**)(arg0 + 0x4), 0);
            }
        }
    }

    u8* def2 = *(u8**)(arg0 + 0x9C);
    if (def2 != NULL && ((*(u32*)(def2 + 0x80) >> 27) & 0xF) != 0) {
        TriggerDoAction(def2 + 0x80, arg0, NULL);
    }
    if (((*(u32*)(arg0 + 0x50) >> 27) & 0xF) != 0) {
        TriggerDoAction(arg0 + 0x50, arg0, NULL);
    }
}

// 0x8005F2AC
// Updates the object's locator effect: picks a locator (by index from +0xC2A, either one of the
// model locators or a record at +0x98 + idx*0xC8), then flags the emitter at *(+0xC24) and
// calls its virtual slot 0x14 with the locator transform.
void fn_8005F2AC(u8* arg0) {
    f32 loc[12];     // locator record copy (sp+0x38 .. 0x68)
    f32 sp18[4];
    f32 sp28[3];
    f32 sp8[3];

    if (*(u8*)(arg0 + 0xBDC) == 0) return;

    u16 flags = *(u16*)(arg0 + 0xC28);
    s32 idx = (flags & 1) ? *(u8*)(arg0 + 0xC2A) : -1;
    s32 have;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(arg0)) {
            fn_801B55F4(arg0, (u8*)loc, idx);
            have = 1;
        } else {
            have = 0;
        }
    } else {
        s32 idx2 = (flags & 2) ? *(u8*)(arg0 + 0xC2A) : -1;
        if (idx2 != -1) {
            u8* rec = *(u8**)(arg0 + 0x98) + idx2 * 0xC8;
            have = 1;
            for (s32 k = 0; k < 12; k++) loc[k] = *(f32*)(rec + 0x38 + k * 4);
        } else {
            have = 0;
        }
    }
    if (have == 0) return;

    u8** holder = *(u8***)(arg0 + 0xC24);
    if (holder == NULL) return;
    u8* em = *holder;
    if (em == NULL) return;

    *(u32*)(em + 0x8) |= 2;
    sp8[2] = 1.0f;
    sp8[1] = 1.0f;
    sp8[0] = 1.0f;
    sp28[0] = 1.0f;
    sp28[1] = 1.0f;
    sp28[2] = 1.0f;
    fn_8014474C((u8*)loc, sp18);
    u8* em2 = **(u8***)(arg0 + 0xC24);
    (*(VFn_pfff*)(*(u8**)em2 + 0x14))(em2, &loc[9], sp28, sp18);
}

}  // extern "C"
