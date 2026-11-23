// Misc decompiled functions: audio SoundAt, lab/UI helpers, EulerToQuat, lightning, ...
#include <stdint.h>

#include "types.h"
#include "game/part.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" u8* fn_801B55A8(u8* ent);                           // 0x801B55A8 (defined with C++ linkage; returns the locator count as an integer value)

extern "C++" u8* fn_801330E4(u8* world, u8* zone);                // 0x801330E4 (defined with C++ linkage)

typedef void (*VFn_ps)(void*, s32);
typedef void (*VFn_pss)(void*, s32, s32);

extern "C" {

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

u8* fn_80208CE8(void);                                          // 0x80208CE8
s32 fn_802099B8(u8* a, s32 b, u8* c, s32 d);                    // 0x802099B8
s32 fn_801FF154(u8* audio, void* desc);                         // 0x801FF154
s32 fn_801FF090(u8* audio, void* desc);                         // 0x801FF090
s32 fn_801FE52C(u8* audio, void* desc, s32 c);                  // 0x801FE52C
u32 RandomU32(void);                                            // 0x80143460
void ScopedLockCtor(ScopedLock* l);                             // 0x801F5384
void ScopedLockDtor(ScopedLock* l, s32 flags);                  // 0x801F53B0
void fn_8004BA50(u8* chr);                                      // 0x8004BA50
void fn_8004B938(u8* chr);                                      // 0x8004B938
s32 fn_8004EA38(u8* arg0, s32 arg1, u8* arg2);                             // 0x8004EA38
void fn_8020E668(u8* tbl, s32 a, s32 b);                        // 0x8020E668
s32 fn_8020E7C8(s32 i);                                         // 0x8020E7C8
void fn_8020E610(u8* tbl, s32 a, u8* b);                        // 0x8020E610
void fn_8020E650(u8* tbl, u8* a, void* b, u8* c, void* d);      // 0x8020E650
void* fn_801D40E0(s32 n);                                       // 0x801D40E0
u8* fn_80049628(u8* p, s32 a2);                            // 0x80049628

void* ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
void* ScreenSetFindWidget(void* set, u32 hash);                 // 0x80198E34
void* ScreenSetFindElement(void* set, u32 hash);                // 0x80198F18
void* UIWidgetFindChild(void* widget, u32 hash);                // 0x80199AEC
void* fn_80198F1C(void* set, u32 hash);                         // 0x80198F1C
s32 fn_8019BBA4(void* el, void* aux, s32 n);                    // 0x8019BBA4
void* fn_80209814(void* v);                                     // 0x80209814
void UISetText(void* el, const char* text, s32 a);              // 0x8019EFC8
s32 PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);     // 0x80209E70
void GameFlowSignal(s32 id, s32 a, s32 b);                      // 0x8000C7F4
s32 fn_8015BA30(void* p);                                       // 0x8015BA30
void fn_8014DE68(void);                                         // 0x8014DE68
void fn_801567CC(f32 a, f32 b, f32 c, f32 d);                   // 0x801567CC
void fn_801566E8(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);     // 0x801566E8
void fn_80154E54(u8* m);  // 0x80154E54
void fn_80155D70(void);                                         // 0x80155D70
void fn_8016DDAC(u8* cam);                                      // 0x8016DDAC
u8* fn_801186FC(u8* rec);                                       // 0x801186FC
BOOL SavePartTierSeen(u8* s, u32 part, s32 tier);               // progress.cpp
void* fn_8011AD88(void* def, s32 flag);                         // 0x8011AD88
void EntryComputeRequiredRank(u8* e);                           // monster.cpp
s32 LabExperimentGrade(u8* lab);                                // lab_object.cpp
void fn_801E93EC(u8* list, u8* entry);                          // 0x801E93EC
void* memset(void* p, int c, unsigned long n);
f32 cosf(f32 x);                                                // 0x8033EC98 (MSL)
f32 sinf(f32 x);                                                // 0x8033F0A4 (MSL)
void fn_801B55F4(u8* obj, u8* out, s32 idx);                    // 0x801B55F4
s32 fn_801ED368(u8* ref);                                       // 0x801ED368
BOOL fn_801F5220(void);                                         // 0x801F5220
void fn_801E8D74(void* out, void* list, void* owner, void* anim);  // 0x801E8D74
s32 fn_800ED944(u8* p, f32 dt);                                         // 0x800ED944 (lightning callback)
u32 StringHash(const char* s);                                  // 0x801EB704
u8* GetGlobalSettings(void);                                    // 0x801D2BF8
void* IdPoolNext(void* pool);                                   // 0x801E6EB0
void ZoneSlotInit(void* slot);                                  // 0x801F2B40
s32 fn_80133134(void* world, s32 zone);                         // 0x80133134
void fn_8013786C(u8* world, s32 slot);                          // 0x8013786C
void fn_801360BC(u8* world, s32 slot, s32 arg);                 // 0x801360BC
s32 fn_801E5E90(u8* pool, void* idp);                           // 0x801E5E90
void fn_801F3BB8(void);                                         // 0x801F3BB8
void fn_801F5E48(void* a, s32 b);                               // 0x801F5E48
void fn_801F5E4C(void* a, s32 b);                               // 0x801F5E4C
void fn_801F5B30(u8* slot);                                     // 0x801F5B30
}

struct StringTable;
extern "C++" StringTable* gStringTable;                         // 0x805FB910
extern "C++" void* gScreenSets;                                 // 0x805FB654
extern void* lbl_805FBBAC;                                      // 0x805FBBAC (voice SFX handle)
extern const char lbl_805F8200[];                               // 0x805F8200 (empty string)
extern u8 sRendDLs[];                                           // 0x80401108 (display-list records, 0x94 bytes each)
extern s32 gViewportIndex;                                      // 0x805FB4BC
extern s32 gFrameBudget;                                        // 0x805FB4C8
extern u8* lbl_80412AF0[];                                      // 0x80412AF0 (camera stack)
extern s32 lbl_805FB600;                                        // 0x805FB600 (camera stack depth)
extern void* lbl_805FB5F8;                                      // 0x805FB5F8 (current camera)
extern s32 lbl_805FB5FC;                                        // 0x805FB5FC (camera set counter)
extern u8 lbl_804DD568[];                                       // 0x804DD568 (ring: +0 flag, +8 head, +0xC tail, +0x10 cap, +0x14 base, 0x400-byte slots)
extern u8 lbl_804FB5E0[];                                       // 0x804FB5E0
extern u8* lbl_805FB95C;                                        // 0x805FB95C (current movie slot)
extern void* lbl_805FB964;                                      // 0x805FB964 (movie player)
extern void* lbl_805FB968;                                      // 0x805FB968 (current target)
extern s32 lbl_805FB970;                                        // 0x805FB970
extern s32 lbl_805FB974;                                        // 0x805FB974
extern s32 lbl_805FB994;                                        // 0x805FB994
extern s32 lbl_805FB9D8;                                        // 0x805FB9D8
extern s32 lbl_805FB9AC;                                        // 0x805FB9AC
extern s32 lbl_805F97F8;                                        // 0x805F97F8
extern void* gLab;                                              // 0x805FBBC8
extern u8 lbl_803E6B28[];                                       // 0x803E6B28 (list entry record: +0 kind, +4 index)
extern u8 lbl_803E6B34[];                                       // 0x803E6B34 (list entry record: +0 kind, +4 index)
extern u16 lbl_805FB268;                                        // 0x805FB268
extern s32 lbl_805F8F24;                                        // 0x805F8F24
extern s32 lbl_805FB26C;                                        // 0x805FB26C
extern s32 lbl_805FB270;                                        // 0x805FB270
extern s32 lbl_805FB274;                                        // 0x805FB274
extern s32 lbl_805FB278;                                        // 0x805FB278
extern s32 lbl_805FB27C;                                        // 0x805FB27C
extern s32 lbl_805FB280;                                        // 0x805FB280
extern void* lbl_805FB284;                                      // 0x805FB284
extern s32 lbl_805FB288;                                        // 0x805FB288
extern u8* lbl_805FBBE0;                                        // 0x805FBBE0 (save block pointer)
extern u8* lbl_805FB8D8;                                        // 0x805FB8D8 (effect pool: 0xFF entries of 0x80 bytes, free list at +0x7F80, count at +0x837C)
extern u8 gWorldState[];                                        // 0x803E6D00
extern const char lbl_8036045C[];                               // 0x8036045C (effect name string)
extern u32 lbl_805FB1EC;                                        // 0x805FB1EC (StringHash cache)
extern u8 lbl_805FB1F0;                                         // 0x805FB1F0 (cache valid flag)
extern u8 gIdPool[];                                            // 0x8048A8F0
extern "C++" s32 gZoneResourceSize;                             // 0x805FB94C
extern u8 gAudio[];
extern u8 lbl_805FBBB2[];

extern "C" {

struct SoundDesc {
    s32 id;
    void* obj;
    s32 handle;
    f32 pitch;
    f32 vol;
    s32 arg3;
    s32 flags;
    f32 f24;
    u8 b28;
    u8 b29;
    f32 minDist;
    f32 maxDist;
};

// 0x80209ABC
// Picks a random variant from a sound table (entries of 0x30 bytes), randomizes volume
// and submits it to the audio manager (positional by handle, by object, or global).
s32 PlaySoundAt(u8* arg0, s32 arg1, u8* arg2, s32 arg3) {
    if (arg0 == NULL || *(u8**)arg0 == NULL || *(s32*)fn_80208CE8() == 0) return -1;
    u8* hdr = *(u8**)arg0;
    if (hdr != NULL && *(u16*)(hdr + 0xC) != 0) return fn_802099B8(arg0, arg1, arg2, arg3);
    if (lbl_805FBBB2[0] != 0) return -1;
    f32 jitter = (f32)*(u8*)(arg0 + 7);
    s32 count = *(u8*)(arg0 + 4);
    u32 idx;
    if (count > 0) {
        if ((s32)*(u8*)(arg0 + 0x17) <= count) {
            f32 hi = ((f32)count + *(f32*)(fn_80208CE8() + 4)) - 1.0f;
            f32 f = 0.0f + ((hi - 0.0f) * (2.3283064e-10f * (f32)RandomU32()));
            if (f > (f32)(count - 1)) idx = *(u8*)(arg0 + 0x17);
            else if (f < (f32)*(u8*)(arg0 + 0x17)) idx = (u32)f;
            else idx = (u32)f + 1;
            *(u8*)(arg0 + 0x17) = (u8)idx;
        } else {
            idx = (u32)(0.0f + (((f32)count - 0.0f) * (2.3283064e-10f * (f32)RandomU32())));
        }
    } else {
        idx = 0;
    }
    u8* ent = arg0 + idx * 0x30;
    f32 vol = 0.0f;
    f32 base = 1.2f * ((f32)*(u8*)(ent + 6) / 255.0f);
    if (!(base < 0.0f)) {
        f32 v = 1.2f;
        if (!(base > 1.2f)) v = 1.2f * ((f32)*(u8*)(ent + 6) / 255.0f);
        vol = v;
    }
    jitter = jitter / 255.0f;
    f32 nj = -jitter;
    f32 finalVol = vol + (nj + ((jitter - nj) * (2.3283064e-10f * (f32)RandomU32())));
    if (*(s32*)ent == 0) return -1;
    SoundDesc d;
    d.id = 0;
    d.obj = NULL;
    d.handle = 0;
    d.pitch = 1.0f;
    d.vol = 1.0f;
    d.arg3 = 0;
    d.flags = 0;
    d.f24 = 0.0f;
    d.b28 = 0;
    d.b29 = 0;
    d.minDist = 5.0f;
    d.maxDist = 30.0f;
    d.id = *(s32*)ent;
    d.handle = arg1;
    d.obj = arg2;
    d.minDist = (f32)*(u8*)(ent + 0x15);
    d.maxDist = (f32)*(u8*)(ent + 0x16);
    d.pitch = *(f32*)(ent + 8);
    d.vol = finalVol;
    d.arg3 = arg3;
    d.b28 = *(u8*)(ent + 0x2D);
    d.flags = (s32)*(u8*)(ent + 0x14);
    d.f24 = *(f32*)(ent + 0x10);
    d.b29 = *(u8*)(ent + 0x2E);
    if (arg1 != 0) return fn_801FF154(gAudio, &d);
    if (arg2 != NULL) return fn_801FF090(gAudio, &d);
    return fn_801FE52C(gAudio, &d, 0);
}

// 0x8004C718
// Re-targets slot idx of the character controller table to the object in arg2: tears
// down the old one, rebinds the five linked slots and re-registers the pairs.
void fn_8004C718(u8* arg0, s32 arg1, u8* arg2) {
    u8 flags[5];
    flags[0] = 0;
    flags[1] = 0;
    u8* var_r29 = NULL;
    flags[2] = 0;
    flags[3] = 0;
    flags[4] = 0;
    u8* tbl = *(u8**)(arg0 + 0xC4C);
    if (tbl != NULL) var_r29 = *(u8**)(tbl + arg1 * 4);
    s32 var_r28 = -1;
    s32 cf = *(s32*)(*(u8**)(arg0 + 4) + 0x20);
    if ((cf & 1) || (cf & 2)) {
        fn_8004BA50(arg0);
        u8* slot = arg0 + arg1 * 4;
        u8* old = *(u8**)(slot + 0xC50);
        if (old != NULL) {
            ScopedLock lock;
            ScopedLockCtor(&lock);
            (*(VFn_ps*)(*(u8**)old + 0x18))(old, 0);
            ScopedLockDtor(&lock, -1);
            fn_8004EA38(*(u8**)(slot + 0xC50), 1, NULL);  // r5 leftover in asm
        }
    }
    u8* newObj;
    if (arg2 != NULL) {
        newObj = *(u8**)(arg2 + 0x20);
        if (newObj != NULL) newObj -= 0x264;
    } else {
        newObj = NULL;
    }
    s32 off = arg1 * 4;
    u8* slotp = arg0 + off;
    *(u8**)(slotp + 0xC50) = newObj;
    if (var_r29 != NULL) {
        u8* p8 = *(u8**)(var_r29 + 8);
        if (p8 != NULL) {
            var_r28 = *(s32*)(*(u8**)(p8 + 4) + 4);
            fn_8020E668(*(u8**)(arg0 + 0xC4C), var_r28, arg1);
        }
        for (s32 i = 0; i < 5; i++) {
            if (*(s32*)(var_r29 + 0xC) != 0) {
                flags[i] = 1;
                fn_8020E668(*(u8**)(arg0 + 0xC4C), arg1, i);
            }
            var_r29 += 4;
        }
    } else {
        for (s32 i = 0; i < 5; i++) {
            if (arg1 == fn_8020E7C8(i)) flags[i] = 1;
        }
    }
    fn_8020E610(*(u8**)(arg0 + 0xC4C), arg1, arg2);
    if (var_r28 == -1) var_r28 = fn_8020E7C8(arg1);
    if (var_r28 != -1) {
        u8* t = *(u8**)(arg0 + 0xC4C);
        u8* a = *(u8**)(t + var_r28 * 4);
        u8* b = *(u8**)(t + off);
        if (a != NULL && b != NULL) {
            void* m = fn_801D40E0(8);
            void* v27 = m;
            if (m != NULL) v27 = fn_80049628((u8*)m, (s32)(intptr_t)a);  // lint:ok 32-bit slot
            void* m2 = fn_801D40E0(8);
            void* v5 = m2;
            if (m2 != NULL) v5 = fn_80049628((u8*)m2, (s32)(intptr_t)b);  // lint:ok 32-bit slot
            fn_8020E650(*(u8**)(arg0 + 0xC4C), a, v5, b, v27);
        }
    }
    s32 wi = 0;
    for (s32 i = 0; i < 5; i++) {
        if (flags[i] != 0) {
            u8* t = *(u8**)(arg0 + 0xC4C);
            u8* a = *(u8**)(t + off);
            u8* b = *(u8**)(t + wi);
            if (a != NULL && b != NULL) {
                void* m = fn_801D40E0(8);
                void* v27 = m;
                if (m != NULL) v27 = fn_80049628((u8*)m, (s32)(intptr_t)a);  // lint:ok 32-bit slot
                void* m2 = fn_801D40E0(8);
                void* v5 = m2;
                if (m2 != NULL) v5 = fn_80049628((u8*)m2, (s32)(intptr_t)b);  // lint:ok 32-bit slot
                fn_8020E650(*(u8**)(arg0 + 0xC4C), a, v5, b, v27);
            }
        }
        wi += 4;
    }
    u8* cur = *(u8**)(slotp + 0xC50);
    if (cur != NULL) {
        s32 cf2 = *(s32*)(*(u8**)(arg0 + 4) + 0x20);
        if ((cf2 & 1) || (cf2 & 2)) {
            if (!(*(s32*)(*(u8**)(cur + 4) + 0x20) & 0x200000) && !(*(s32*)(cur + 0xB8) & 1)) {
                (*(VFn_pss*)(*(u8**)cur + 0x30))(cur, 0, 0);
            }
            u8* o2 = *(u8**)(slotp + 0xC50);
            ScopedLock lock2;
            ScopedLockCtor(&lock2);
            (*(VFn_ps*)(*(u8**)o2 + 0x14))(o2, 1);
            ScopedLockDtor(&lock2, -1);
        }
        u8* st = *(u8**)(*(u8**)(slotp + 0xC50) + 4);
        *(u32*)(st + 0x20) = *(u32*)(st + 0x20) & ~1u;
    }
    s32 cf3 = *(s32*)(*(u8**)(arg0 + 4) + 0x20);
    if ((cf3 & 1) || (cf3 & 2)) fn_8004B938(arg0);
}

// Shows string table entry strIdx on the hint element, speaks the voice line and plays sfx.
static void HintShowText(u8* el, void* aux, u32 strIdx, u32 sndHash) {
    u8* strs = *(u8**)gStringTable;
    const char* text = lbl_805F8200;
    if (strs != NULL && *(u32*)((u8*)gStringTable + 0xC) > strIdx) text = *(const char**)(strs + strIdx * 4);
    UISetText(el, text, 0);
    fn_8019BBA4(el, aux, 0xA);
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySound(sndHash, 0, 0, 0, 0, 0);
}

// 0x800704AC
// Hint-text sequencer: advances the timer at +0x6C by dt and, as it crosses 0/1/2/3/4,
// shows the matching hint line (strings 0x583/0x582/0x581/0x584), then signals game flow.
void fn_800704AC(u8* arg0, f32 dt) {
    f32 t = *(f32*)(arg0 + 0x6C);
    f32 tn = t + dt;
    void* set = ScreenSetsCurrent(gScreenSets);
    u8* el = NULL;
    void* aux = NULL;
    if (set != NULL) {
        void* w = ScreenSetFindWidget(set, 0xC62FBC4Eu);
        if (w != NULL) {
            el = (u8*)UIWidgetFindChild(w, 0xD07D25AEu);
            aux = fn_80198F1C(set, 0x89F1B913u);
        }
    }
    f32 cur = *(f32*)(arg0 + 0x6C);
    if (0.0f == cur) {
        if (el != NULL) {
            (*(VFn_ps*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
            HintShowText(el, aux, 0x583, 0xB3BFC7C1u);
        }
    } else if (cur <= 1.0f && tn >= 1.0f) {
        if (el != NULL) HintShowText(el, aux, 0x582, 0xB3BFC7C1u);
    } else if (*(f32*)(arg0 + 0x6C) <= 2.0f && tn >= 2.0f) {
        if (el != NULL) HintShowText(el, aux, 0x581, 0xB3BFC7C1u);
    } else if (*(f32*)(arg0 + 0x6C) <= 3.0f && tn >= 3.0f) {
        if (el != NULL) HintShowText(el, aux, 0x584, 0x8EF2260Cu);
    } else if (*(f32*)(arg0 + 0x6C) <= 4.0f && tn >= 4.0f) {
        if (el != NULL) (*(VFn_ps*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        GameFlowSignal(0xF15918A6u, 0, 0);
    }
    *(f32*)(arg0 + 0x6C) = tn;
}

// 0x80021F2C
// Same hint sequencer for the dig challenge (timer at +0x70, hint element looked up directly).
void fn_80021F2C(u8* arg0, f32 dt) {
    f32 t = *(f32*)(arg0 + 0x70);
    f32 tn = t + dt;
    void* set = ScreenSetsCurrent(gScreenSets);
    u8* el = NULL;
    void* aux = NULL;
    if (set != NULL) {
        el = (u8*)ScreenSetFindElement(set, 0xD07D25AEu);
        aux = fn_80198F1C(set, 0x89F1B913u);
    }
    f32 cur = *(f32*)(arg0 + 0x70);
    if (0.0f == cur) {
        if (el != NULL) {
            (*(VFn_ps*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
            HintShowText(el, aux, 0x583, 0xB3BFC7C1u);
        }
    } else if (cur <= 1.0f && tn >= 1.0f) {
        if (el != NULL) HintShowText(el, aux, 0x582, 0xB3BFC7C1u);
    } else if (*(f32*)(arg0 + 0x70) <= 2.0f && tn >= 2.0f) {
        if (el != NULL) HintShowText(el, aux, 0x581, 0xB3BFC7C1u);
    } else if (*(f32*)(arg0 + 0x70) <= 3.0f && tn >= 3.0f) {
        if (el != NULL) HintShowText(el, aux, 0x584, 0x8EF2260Cu);
    } else if (*(f32*)(arg0 + 0x70) <= 4.0f && tn >= 4.0f) {
        if (el != NULL) (*(VFn_ps*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        GameFlowSignal(0xF15918A6u, 0, 0);
    }
    *(f32*)(arg0 + 0x70) = tn;
}

// 0x80170044
// Pops the camera stack, sets the projection and builds the inverse view matrix from the
// camera's 3x3 basis (+0x00..0x20, rows normalised) and translation (+0x24..0x2C) into +0x90..0xBC.
void fn_80170044(void* unused) {
    s32 depth = lbl_805FB600 - 1;
    lbl_805FB600 = depth;
    u8* cam = lbl_80412AF0[depth];
    lbl_805FB5F8 = cam;
    if (cam == NULL) return;
    if (fn_8015BA30(sRendDLs + gViewportIndex * 0x94) + 0x12A0 > gFrameBudget) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + gViewportIndex * 0x94);
    }
    if (*(s32*)(cam + 0x188) != 0) {
        fn_801567CC(*(f32*)(cam + 0x180), *(f32*)(cam + 0x184), *(f32*)(cam + 0x194), *(f32*)(cam + 0x198));
    } else {
        f32 hw = *(f32*)(cam + 0x180);
        f32 hh = *(f32*)(cam + 0x184);
        fn_801566E8(-hw, hw, -hh, hh, *(f32*)(cam + 0x194), *(f32*)(cam + 0x198));
    }
    lbl_805FB5FC += 1;
    f32 m0 = *(f32*)(cam + 0x0);
    f32 m1 = *(f32*)(cam + 0x4);
    f32 m3 = *(f32*)(cam + 0xC);
    f32 m4 = *(f32*)(cam + 0x10);
    f32 m5 = *(f32*)(cam + 0x14);
    f32 m6 = *(f32*)(cam + 0x18);
    f32 m7 = *(f32*)(cam + 0x1C);
    f32 m2 = *(f32*)(cam + 0x8);
    f32 m8 = *(f32*)(cam + 0x20);
    f32 invB = 1.0f / ((m5 * m5) + ((m3 * m3) + (m4 * m4)));
    f32 invA = 1.0f / ((m2 * m2) + ((m0 * m0) + (m1 * m1)));
    f32 invC = 1.0f / ((m8 * m8) + ((m6 * m6) + (m7 * m7)));
    f32 c01 = m1 * invB;
    f32 c11 = m4 * invB;
    f32 c00 = m0 * invA;
    f32 c10 = m3 * invA;
    f32 c21 = m7 * invB;
    f32 c20 = m6 * invA;
    f32 c02 = m2 * invC;
    f32 c12 = m5 * invC;
    f32 c22 = m8 * invC;
    f32 tx = -*(f32*)(cam + 0x24);
    f32 ty = -*(f32*)(cam + 0x28);
    f32 tz = -*(f32*)(cam + 0x2C);
    *(f32*)(cam + 0x90) = c00;
    *(f32*)(cam + 0x94) = c10;
    *(f32*)(cam + 0x98) = c20;
    *(f32*)(cam + 0x9C) = c01;
    *(f32*)(cam + 0xA0) = c11;
    *(f32*)(cam + 0xA4) = c21;
    *(f32*)(cam + 0xA8) = c02;
    *(f32*)(cam + 0xAC) = c12;
    *(f32*)(cam + 0xB0) = c22;
    f32 p6 = (c10 * tx) + (c11 * ty);
    f32 p9 = c00 * tx;
    f32 p9b = p9 + (c01 * ty);
    f32 p11 = c02 * tz;
    f32 p3 = (c20 * tx) + (c21 * ty);
    f32 tB4 = p11 + p9b;
    f32 p7 = c12 * tz;
    f32 p5 = c22 * tz;
    *(f32*)(cam + 0xB4) = tB4;
    f32 tB8 = p7 + p6;
    *(f32*)(cam + 0xB8) = tB8;
    f32 tBC = p5 + p3;
    *(f32*)(cam + 0xBC) = tBC;
    fn_80154E54(cam + 0x90);
    fn_80155D70();
    fn_8016DDAC(cam);
}

typedef s32 (*VFn_ret)(void*);

// Number of queued slots in the ring at lbl_804DD568 (0 if empty/invalid).
static s32 RingCount(void) {
    s32 head = *(s32*)(lbl_804DD568 + 8);
    s32 bad = 1;
    if (head >= 0 && *(s32*)(lbl_804DD568 + 0xC) >= 0) bad = 0;
    if (bad != 0) return 0;
    s32 t = *(s32*)(lbl_804DD568 + 0xC);
    if (t < head) t += *(s32*)(lbl_804DD568 + 0x10);
    return (t - head) + 1;
}

// 0x801F357C
// Movie streaming pump: keeps a ring of 0x400-byte slots filled while the movie player is
// ready, throttling (lbl_805FB994) when the ring runs low/high, and grabs the next free slot.
void fn_801F357C(void) {
    if (lbl_805FB970 == 0) return;
    if (lbl_805FB974 == 0) return;
    if (lbl_805FB968 == NULL) return;
    if ((uintptr_t)lbl_805FB968 == 2) return;
    s32 ok = 0;
    if (lbl_805FB964 != NULL && (*(VFn_ret*)(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0) ok = 1;
    if (ok == 0) return;
    if (lbl_805FB994 != 0) {
        s32 ok2 = 0;
        if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL &&
            (*(VFn_ret*)(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0) ok2 = 1;
        if (ok2 == 0) lbl_805FB994 = 0;
    }
    if (lbl_805FB994 != 0) {
        lbl_805FB994 = lbl_805FB994 + 1;
        if (RingCount() < 2) {
            lbl_805FB994 = 0;
            fn_801F3BB8();
        } else if (lbl_805FB95C != NULL) {
            return;
        }
    }
    if (lbl_805FB9D8 == 0) fn_801F5E4C(lbl_804FB5E0, 1);
    s32 n = lbl_805FB9D8 + 1;
    lbl_805FB9D8 = n;
    if (RingCount() > 0x1E && lbl_805FB994 == 0) lbl_805FB994 = 1;
    if (lbl_805FB95C != NULL) {
        lbl_805FB9D8 = n - 1;
        if (n - 1 == 0) fn_801F5E48(lbl_804FB5E0, 1);
        return;
    }
    s32 cap = *(s32*)(lbl_804DD568 + 0x10);
    s32 next = (*(s32*)(lbl_804DD568 + 0xC) + 1) % cap;
    s32 head = *(s32*)(lbl_804DD568 + 8);
    u8* slot;
    if (head == next) {
        slot = NULL;
    } else {
        *(s32*)(lbl_804DD568 + 0xC) = next;
        if (head < 0) {
            *(s32*)(lbl_804DD568 + 8) = next;
            *(s32*)(lbl_804DD568 + 0) = 1;
        }
        s32 t = *(s32*)(lbl_804DD568 + 0xC);
        s32 valid;
        if (t < 0 || t >= *(s32*)(lbl_804DD568 + 0x10)) {
            valid = 0;
        } else {
            s32 h = *(s32*)(lbl_804DD568 + 8);
            s32 lo = t;
            s32 hi = t;
            if (t < h) {
                lo = t + *(s32*)(lbl_804DD568 + 0x10);
                hi = t + *(s32*)(lbl_804DD568 + 0x10);
            }
            valid = 0;
            if (h <= lo && lo <= hi) valid = 1;
        }
        if (valid != 0) slot = *(u8**)(lbl_804DD568 + 0x14) + (t << 10);
        else slot = NULL;
    }
    lbl_805FB95C = slot;
    if (slot != NULL) {
        fn_801F5B30(slot);
        *(s32*)(lbl_805FB95C + 4) = lbl_805F97F8;
        *(s32*)(lbl_805FB95C + 0x14) = lbl_805FB9AC;
    }
    s32 m = lbl_805FB9D8 - 1;
    lbl_805FB9D8 = m;
    if (m == 0) fn_801F5E48(lbl_804FB5E0, 1);
}

// 0x8011A04C
// Rebuilds the lab info-panel state for the selected list entries (a part at lbl_803E6B28 and
// an experiment at lbl_803E6B34): name text ids, icons, grade, quality percentage, tier text.
void fn_8011A04C(void) {
    lbl_805FB268 = 0;
    lbl_805F8F24 = -1;
    lbl_805FB26C = 0;
    lbl_805FB270 = 0;
    lbl_805FB274 = 0;
    lbl_805FB278 = 0;
    lbl_805FB27C = 0;
    lbl_805FB280 = 0;
    lbl_805FB284 = NULL;
    lbl_805FB288 = 0;
    s32 idxA = *(s32*)(lbl_803E6B28 + 4);
    if (idxA != -1 && *(s32*)(lbl_803E6B34 + 4) != -1) {
        s32 idxB = *(s32*)(lbl_803E6B34 + 4);
        u8* data = gGameDb->data;
        u32 part = *(u32*)((u8*)gLab + 0x58);
        u8* ea = *(u8**)(data + 0x4F00) + idxA * 0x10;
        u8* eb = *(u8**)(data + 0x5328) + idxB * 0x18;
        lbl_805FB26C = TextIdFromHash(*(u32*)(ea + 4));
        lbl_805FB270 = TextIdFromHash(*(u32*)(eb + 4));
        lbl_805FB27C = (s32)(intptr_t)fn_801186FC(lbl_803E6B28);  // lint:ok 32-bit slot
        lbl_805FB280 = (s32)(intptr_t)fn_801186FC(lbl_803E6B34);  // lint:ok 32-bit slot
        u8* ep = *(u8**)(gGameDb->data + 0x6C18) + part * 0x24;
        PartDef def;
        PartDbGetDef(&def, gGameDb, *(s32*)(ep + 0x20));
        u8 tierRaw = *(u8*)(eb + 0x13);
        s32 ti = (s8)tierRaw - 1;
        s16 pct[4];
        pct[0] = (s16)def.unk44;
        pct[1] = (s16)def.unk42;
        pct[2] = (s16)def.unk40;
        pct[3] = *(s16*)def.unk48;
        s16 pv = pct[ti];
        lbl_805FB268 = (u16)(((s8)def.unk4C[0] * pv) / 100);
        lbl_805F8F24 = LabExperimentGrade((u8*)gLab);
        if (SavePartTierSeen(lbl_805FBBE0, part, tierRaw) != 0) {
            u32 names[4];
            names[0] = def.tierName[0];
            names[1] = def.tierName[1];
            names[2] = def.tierName[2];
            names[3] = def.tierName[3];
            lbl_805FB274 = TextIdFromHash(names[ti]);
            lbl_805FB284 = fn_8011AD88(&def, 0);
        }
        EntryComputeRequiredRank((u8*)gLab);
        return;
    }
    if (idxA != -1) {
        u8* ea = *(u8**)(gGameDb->data + 0x4F00) + idxA * 0x10;
        lbl_805FB26C = TextIdFromHash(*(u32*)(ea + 4));
        lbl_805FB27C = (s32)(intptr_t)fn_801186FC(lbl_803E6B28);  // lint:ok 32-bit slot
        return;
    }
    if (*(s32*)(lbl_803E6B34 + 4) != -1) {
        u8* eb = *(u8**)(gGameDb->data + 0x5328) + *(s32*)(lbl_803E6B34 + 4) * 0x18;
        lbl_805FB270 = TextIdFromHash(*(u32*)(eb + 4));
        lbl_805FB280 = (s32)(intptr_t)fn_801186FC(lbl_803E6B34);  // lint:ok 32-bit slot
    }
}

typedef u8 (*PoolCb)(u8* self, void* arg, f32 dt);

// Resolves a pool handle (low byte = slot, high byte = generation) to its entry, or NULL.
static u8* PoolEntry(u16 h) {
    u8* pool = lbl_805FB8D8;
    u32 idx = h & 0xFF;
    if (idx < 0xFF && (h & 0xFF00) != 0) return pool + (idx << 7);
    return NULL;
}

// Runs a pool entry's callback once (state 0x40 while running, 0x80 when done).
static void PoolRun(u8* e) {
    e[0x7F] = 0x40;
    if (*(PoolCb*)(e + 0x6C) != NULL) {
        *(f32*)(e + 0x70) = *(f32*)(e + 0x70) + 0.0f;
        e[0x7E] = (*(PoolCb*)(e + 0x6C))(e, *(void**)(e + 0x68), 0.0f);
    }
    e[0x7F] = 0x80;
}

// 0x801E839C
// Updates the effect list owned by arg0: runs every pending entry of its chain, hands finished
// ones to fn_801E93EC, then frees all pool entries owned by arg0 back to the free list.
void fn_801E839C(u8* arg0) {
    *(u16*)(arg0 + 6) = *(u16*)(arg0 + 6) & 0xFFFE;
    if (*(u16*)(arg0 + 4) != 0) {
        u8* e = PoolEntry(*(u16*)(arg0 + 0));
        while (e != NULL) {
            if (e[0x7F] != 0x80) PoolRun(e);
            e = PoolEntry(*(u16*)(e + 0x78));
        }
        u16 fl = *(u16*)(arg0 + 6) | 2;
        *(u16*)(arg0 + 6) = fl;
        if (!(fl & 1) && (fl & 2)) {
            e = PoolEntry(*(u16*)(arg0 + 0));
            while (e != NULL) {
                u8* next = PoolEntry(*(u16*)(e + 0x78));
                u8 st = e[0x7F];
                if (st == 0x40 && st != 0x80) PoolRun(e);
                if (st == 0x80) fn_801E93EC(arg0, e);
                e = next;
            }
            *(u16*)(arg0 + 6) = *(u16*)(arg0 + 6) & 0xFFFD;
        }
    }
    u8* pool = lbl_805FB8D8;
    if (pool != NULL) {
        u8* it = pool;
        for (s32 i = 0; i < 0xFF; i++) {
            if (it[0x7F] != 0 && *(u8**)(it + 0x74) == arg0) {
                it[0x7F] = 0x80;
                u16 h = *(u16*)(it + 0x7C);
                s32 valid = 0;
                s32 ok = 0;
                u32 idx = h & 0xFF;
                if (idx < 0xFF && (h & 0xFF00) != 0) ok = 1;
                if (ok != 0 && *(u16*)(pool + (idx << 7) + 0x7C) == h) valid = 1;
                if (valid != 0) {
                    u8* t = pool + ((u32)(h & 0xFF) << 7);
                    memset(t, 0, 0x64);
                    *(s32*)(t + 0x64) = 0;
                    *(s32*)(t + 0x68) = 0;
                    *(s32*)(t + 0x6C) = 0;
                    *(f32*)(t + 0x70) = 0.0f;
                    *(u16*)(t + 0x78) = 0xFFFF;
                    *(u16*)(t + 0x7A) = 0xFFFF;
                    *(u16*)(t + 0x7C) = 0xFFFF;
                    *(u8**)(t + 0x74) = NULL;
                    t[0x7E] = 0;
                    t[0x7F] = 0;
                    if (*(s32*)(pool + 0x837C) + 1 != 0xFF) {
                        s32 n = *(s32*)(pool + 0x837C) + 1;
                        *(s32*)(pool + 0x837C) = n;
                        *(s32*)(pool + n * 4 + 0x7F80) = (s32)(u8)h;
                    }
                }
            }
            it += 0x80;
        }
    }
}

static f32 WrapEuler(f32 v) {
    if (v > -0.0001f && v < 0.0001f) {
        v = 0.0f;
    } else if (v > 4.0f) {
        v = v - 4.0f;
    } else if (v < 0.0f) {
        v = 4.0f + v;
    }
    return v;
}

// 0x80145B00
// Euler angles (units of quarter turns, wrapped to 0..4) to a quaternion: builds the X, Y and Z
// half-angle quaternions and multiplies them as (Z * Y) * X.
void EulerToQuat(Quat* arg0, const Vec3* arg1) {
    f32 ax[3] = {1.0f, 0.0f, 0.0f};
    f32 ay[3] = {0.0f, 1.0f, 0.0f};
    f32 az[3] = {0.0f, 0.0f, 1.0f};

    f32 hx = 1.5707964f * (0.5f * WrapEuler(arg1->x));
    f32 cxw = cosf(hx);
    f32 sx = sinf(hx);
    f32 ax0 = sx * ax[0];
    f32 ax1 = sx * ax[1];
    f32 ax2 = sx * ax[2];

    f32 hy = 1.5707964f * (0.5f * WrapEuler(arg1->y));
    f32 cyw = cosf(hy);
    f32 sy = sinf(hy);
    f32 by0 = sy * ay[0];
    f32 by1 = sy * ay[1];
    f32 by2 = sy * ay[2];

    f32 hz = 1.5707964f * (0.5f * WrapEuler(arg1->z));
    f32 czw = cosf(hz);
    f32 sz = sinf(hz);
    f32 cz0 = sz * az[0];
    f32 cz1 = sz * az[1];
    f32 cz2 = sz * az[2];

    f32 tx = ((cz1 * by2) + ((czw * by0) + (cz0 * cyw))) - (cz2 * by1);
    f32 tw = (((czw * cyw) - (cz0 * by0)) - (cz1 * by1)) - (cz2 * by2);
    f32 ty = (cz2 * by0) + ((cz1 * cyw) + ((czw * by1) - (cz0 * by2)));
    f32 tz = (cz2 * cyw) + (((czw * by2) + (cz0 * by1)) - (cz1 * by0));

    arg0->x = ((ty * ax2) + ((tw * ax0) + (tx * cxw))) - (tz * ax1);
    arg0->y = (tz * ax0) + ((ty * cxw) + ((tw * ax1) - (tx * ax2)));
    arg0->z = (tz * cxw) + (((tw * ax2) + (tx * ax1)) - (ty * ax0));
    arg0->w = (((tw * cxw) - (tx * ax0)) - (ty * ax1)) - (tz * ax2);
}

// 0x800EF048
// Script callback FireLightningAtTarget: picks a locator on the object (by index or by name slot),
// then spawns a lightning effect entry (callback fn_800ED944) in the world's effect list.
void FireLightningAtTarget(void* a0, u32 /*0x30*/ a1, u32 a2, u32 /*0x38*/ a3, f32 a4, f32 a5, u32 /*0x17*/ a6) {
    u8* obj = (u8*)a0;
    u8* rec = (u8*)(uintptr_t)a1;
    u8* ref = (u8*)(uintptr_t)a6;
    u32 hash = a2;
    u32 arg3 = a3;
    f32 loc[12];
    if (obj == NULL) return;
    u16 flags = *(u16*)rec;
    s32 idx;
    if (flags & 1) idx = *(u8*)(rec + 2);
    else idx = -1;
    s32 have;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(obj)) {
            fn_801B55F4(obj, (u8*)loc, idx);
            have = 1;
        } else {
            have = 0;
        }
    } else {
        s32 idx2;
        if (flags & 2) idx2 = *(u8*)(rec + 2);
        else idx2 = -1;
        if (idx2 != -1) {
            have = 1;
            u8* src = *(u8**)(obj + 0x98) + idx2 * 0xC8;
            for (s32 i = 0; i < 12; i++) loc[i] = *(f32*)(src + 0x38 + i * 4);
        } else {
            have = 0;
        }
    }
    if (have == 0 || fn_801ED368(ref) == 0) return;
    if (hash == 0) hash = 0x62AECFDEu;
    if (fn_801F5220() == 0) arg3 = 0;
    s32 refId = *(s32*)ref;
    s32 recId = *(s32*)rec;
    u32 entBuf[0x20];
    u8* ent = (u8*)entBuf;
    *(u16*)(ent + 0x78) = 0xFFFF;
    *(u16*)(ent + 0x7A) = 0xFFFF;
    *(u16*)(ent + 0x7C) = 0xFFFF;
    memset(ent, 0, 0x64);
    *(s32*)(ent + 0x64) = 0;
    *(s32*)(ent + 0x68) = 0;
    *(void**)(ent + 0x6C) = (void*)fn_800ED944;
    *(f32*)(ent + 0x70) = 0.0f;
    *(u16*)(ent + 0x78) = 0xFFFF;
    *(u16*)(ent + 0x7A) = 0xFFFF;
    *(u16*)(ent + 0x7C) = 0xFFFF;
    *(s32*)(ent + 0x74) = 0;
    ent[0x7E] = 0;
    ent[0x7F] = 0;
    *(u32*)(ent + 0x48) = hash;
    *(s32*)(ent + 0x4C) = 0;
    *(s32*)(ent + 0x0) = recId;
    *(s32*)(ent + 0xC) = refId;
    *(f32*)(ent + 0x10) = 0.0f;
    *(f32*)(ent + 0x14) = 0.0f;
    *(f32*)(ent + 0x18) = 1.0f;
    *(f32*)(ent + 0x1C) = 0.0f;
    *(f32*)(ent + 0x20) = 0.0f;
    *(f32*)(ent + 0x24) = 0.0f;
    *(u32*)(ent + 0x44) = arg3;
    *(f32*)(ent + 0x3C) = (0.0f <= a5) ? a5 : 0.0f;
    *(s32*)(ent + 0x4) = 0;
    *(u16*)(ent + 0xA) = 0xFFFF;
    ent[0x8] = 0;
    ent[0x9] = 0;
    *(f32*)(ent + 0x28) = (a4 >= 0.0f) ? a4 : 0.0f;
    *(f32*)(ent + 0x2C) = 0.0f;
    *(f32*)(ent + 0x30) = 0.0f;
    *(f32*)(ent + 0x34) = 0.0f;
    *(f32*)(ent + 0x38) = 0.0f;
    *(s32*)(ent + 0x40) = 0;
    *(s32*)(ent + 0x50) = (a4 < 0.0f ? 1 : 0) | 4;
    if ((s8)lbl_805FB1F0 == 0) {
        lbl_805FB1EC = StringHash(lbl_8036045C);
        lbl_805FB1F0 = 1;
    }
    *(u32*)(ent + 0x64) = lbl_805FB1EC;
    u32 tween[8];
    f32* tw = (f32*)tween;
    tw[1] = 0.0f;
    tw[2] = 0.0f;
    tw[3] = 1.0f;
    tw[4] = 0.0f;
    tw[5] = 0.0f;
    tw[6] = 0.0f;
    u8* wl = *(u8**)(*(u8**)gWorldState + 0x1C8);
    fn_801E8D74(tween, wl + *(u16*)(*(u8**)(obj + 4) + 0x32) * 0xC, obj, ent);
}

// Total number of zone slots: sum of the slot counts in the global settings.
static s32 ZoneSlotTotal(void) {
    s32 a = *(s32*)(GetGlobalSettings() + 0x4C);
    s32 b = *(s32*)(GetGlobalSettings() + 0x34);
    s32 c = *(s32*)(GetGlobalSettings() + 0x50);
    s32 sum = b + a;
    s32 d = *(s32*)(GetGlobalSettings() + 0x54);
    sum = c + sum;
    s32 e = *(s32*)(GetGlobalSettings() + 0x58);
    sum = d + sum;
    s32 f = *(s32*)(GetGlobalSettings() + 0x5C);
    sum = e + sum;
    s32 g = *(s32*)(GetGlobalSettings() + 0x60);
    sum = f + sum;
    s32 h = *(s32*)(GetGlobalSettings() + 0x64);
    sum = g + sum;
    s32 i = *(s32*)(GetGlobalSettings() + 0x38);
    sum = h + sum;
    s32 j = *(s32*)(GetGlobalSettings() + 0x3C);
    sum = i + sum;
    s32 k = *(s32*)(GetGlobalSettings() + 0x40);
    sum = j + sum;
    s32 l = *(s32*)(GetGlobalSettings() + 0x44);
    sum = k + sum;
    s32 m = *(s32*)(GetGlobalSettings() + 0x48);
    return m + (l + sum);
}

// Total number of zone resources (settings counts + 1).
static s32 ZoneResourceTotal(void) {
    s32 sum = *(s32*)(GetGlobalSettings() + 0x80);
    sum = *(s32*)(GetGlobalSettings() + 0x68) + sum;
    sum = sum + *(s32*)(GetGlobalSettings() + 0x84);
    sum = sum + *(s32*)(GetGlobalSettings() + 0x88);
    sum = sum + *(s32*)(GetGlobalSettings() + 0x8C);
    sum = sum + *(s32*)(GetGlobalSettings() + 0x90);
    sum = sum + *(s32*)(GetGlobalSettings() + 0x94);
    sum = sum + *(s32*)(GetGlobalSettings() + 0x98);
    sum = sum + *(s32*)(GetGlobalSettings() + 0x6C);
    sum = sum + *(s32*)(GetGlobalSettings() + 0x70);
    sum = sum + *(s32*)(GetGlobalSettings() + 0x74);
    sum = sum + *(s32*)(GetGlobalSettings() + 0x78);
    return sum + *(s32*)(GetGlobalSettings() + 0x7C) + 1;
}

// 0x801384B0
// (Re)initialises zone slot arg1 of the world: loads its zone resource if needed, then runs the
// per-kind setup (kind 0/2 reset, 1 allocate a fresh id, 3/4 and 6 run the zone scripts).
void fn_801384B0(u8* arg0, s32 arg1, s32 arg2) {
    s32 ok = 0;
    if (arg1 >= 0 && arg1 < ZoneSlotTotal()) ok = 1;
    u8* slot;
    if (ok != 0) slot = *(u8**)(arg0 + 0xB0) + arg1 * 0x30;
    else slot = NULL;
    if (slot == NULL) return;
    s32 ok2 = 0;
    if (arg1 >= 0 && arg1 < ZoneSlotTotal()) ok2 = 1;
    s32 zoneIdx;
    if (ok2 != 0) zoneIdx = *(s32*)(*(u8**)(arg0 + 0xB0) + arg1 * 0x30 + 8);
    else zoneIdx = -2;
    s32 ok3 = 0;
    if (zoneIdx >= 0 && zoneIdx < ZoneResourceTotal()) ok3 = 1;
    u8* zone;
    if (ok3 != 0) zone = *(u8**)(*(u8**)arg0 + 0x1A4) + zoneIdx * gZoneResourceSize;
    else zone = NULL;
    if (zone == NULL) return;
    if (fn_80133134(arg0, *(s32*)(zone + 0x18)) != 0) fn_801330E4(arg0, zone);
    *(s32*)(slot + 0xC) = *(s32*)(slot + 0xC) | 8;
    ScopedLock lock;
    ScopedLockCtor(&lock);
    switch (*(s32*)(slot + 4)) {
    case 0:
    case 2:
        ZoneSlotInit(slot);
        break;
    case 1: {
        void* idv = *(void**)(slot + 0x20);
        fn_801E5E90(gIdPool, &idv);
        *(void**)(slot + 0x20) = IdPoolNext(gIdPool);
        *(s32*)(slot + 0x24) = -1;
        ZoneSlotInit(slot);
        break;
    }
    case 3:
    case 4:
        fn_801360BC(arg0, arg1, arg2);
        break;
    case 6:
        fn_8013786C(arg0, arg1);
        fn_801360BC(arg0, arg1, arg2);
        break;
    default:
        break;
    }
    *(s32*)(slot + 0xC) = *(s32*)(slot + 0xC) & ~8;
    ScopedLockDtor(&lock, -1);
}

// 0x801C4B90
// Trail/ribbon update: pushes the history (two Vec3 + one alpha per entry, shifted by two entries,
// at most 30 entries), then appends the new endpoints (arg1, arg2) and a distance-driven
// coordinate that ping-pongs between 0 and the limit; arg5 is the colour (alpha in the top byte).
void fn_801C4B90(u8* arg0, f32* arg1, f32* arg2, s32 arg3, s32 arg4, u32 arg5) {
    if (*(u32*)(arg0 + 4) & 2) return;
    if (*(u8*)(*(u8**)(*(u8**)arg0 + 0x6C) + 5) != 0) return;
    if (*(f32*)(arg0 + 0x390) <= 0.0f) {
        u32 n = *(u32*)(arg0 + 8);
        if (n >= 0x1E) n = 0x1E;
        for (s32 k = (s32)n - 1; k >= 0; k--) {
            u8* e = arg0 + k * 0x18;
            *(f32*)(e + 0x3C) = *(f32*)(e + 0xC);
            *(f32*)(e + 0x40) = *(f32*)(e + 0x10);
            *(u32*)(e + 0x44) = *(u32*)(e + 0x14);
            *(f32*)(e + 0x48) = *(f32*)(e + 0x18);
            *(f32*)(e + 0x4C) = *(f32*)(e + 0x1C);
            *(f32*)(e + 0x50) = *(f32*)(e + 0x20);
            *(f32*)(arg0 + k * 4 + 0x314) = *(f32*)(arg0 + k * 4 + 0x30C);
        }
        *(f32*)(arg0 + 0x390) = *(f32*)(arg0 + 0x390) + *(f32*)(*(u8**)arg0 + 0x50);
    }
    f32 x = arg1[0];
    f32 px = *(f32*)(arg0 + 0x48);
    *(f32*)(arg0 + 0x18) = x;
    f32 py = *(f32*)(arg0 + 0x4C);
    f32 dx = x - px;
    f32 y = arg1[1];
    f32 pz = *(f32*)(arg0 + 0x50);
    *(f32*)(arg0 + 0x1C) = y;
    f32 dy = y - py;
    f32 dist = 0.0f;
    f32 z = arg1[2];
    f32 dz = z - pz;
    *(f32*)(arg0 + 0x20) = z;
    *(f32*)(arg0 + 0x30) = arg2[0];
    f32 d2 = (dz * dz) + ((dx * dx) + (dy * dy));
    *(f32*)(arg0 + 0x34) = arg2[1];
    *(f32*)(arg0 + 0x38) = arg2[2];
    if (!(d2 <= 0.0f)) dist = sqrtf(d2);
    u32 flags = *(u32*)(arg0 + 4);
    f32 t;
    if (flags & 0x80) t = *(f32*)(arg0 + 0x3C) + (dist * *(f32*)(*(u8**)arg0 + 0x38));
    else if (flags & 0x40) t = *(f32*)(arg0 + 0x3C) - (dist * *(f32*)(*(u8**)arg0 + 0x38));
    else t = 0.0f;
    u8* def = *(u8**)arg0;
    if (t > *(f32*)(*(u8**)(def + 0x6C) + 0xC)) {
        *(u32*)(arg0 + 4) = (*(u32*)(arg0 + 4) & ~0x80u) | 0x40;
        f32 lim = *(f32*)(*(u8**)(def + 0x6C) + 0xC);
        t = lim - (t - lim);
    } else if (t < 0.0f) {
        t = -t;
        *(u32*)(arg0 + 4) = (*(u32*)(arg0 + 4) & ~0x40u) | 0x80;
    }
    f32 alpha = (f32)(arg5 >> 24);
    *(f32*)(arg0 + 0x24) = t;
    *(f32*)(arg0 + 0xC) = t;
    *(f32*)(arg0 + 0x10) = 0.0f;
    *(f32*)(arg0 + 0x28) = 1.0f;
    *(u32*)(arg0 + 0x2C) = arg5;
    *(u32*)(arg0 + 0x14) = arg5;
    *(f32*)(arg0 + 0x310) = alpha;
    *(f32*)(arg0 + 0x30C) = alpha;
    *(u32*)(arg0 + 8) = *(u32*)(arg0 + 8) + 2;
}

}  // extern "C"
