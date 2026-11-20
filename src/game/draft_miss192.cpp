// Missing functions batch 192: file wrapper open, conversation intro, part/limb setup helpers.
#include <math.h>
#include <string.h>

#include "types.h"
#include "game/character.h"
#include "game/part.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

typedef void (*VFnI)(u8*, s32);
typedef void (*VFnIF)(u8*, s32, f32);
typedef void (*VFnF)(u8*, f32);
typedef void (*VFnPP)(u8*, f32*, f32*);

extern "C" {
f32 AnimGetLength(void* anim);                                  // 0x8021EB4C
f32 AnimGetTime(void* anim);                                    // 0x8021EB44
void AudioSetPaused(void* audio, int paused);
void* CharacterFind(s32 id);                                    // 0x800E876C
void ExplorationPromptClear(BOOL animate);
void HudShowConversation(void* hud, s32 conversation);
u32 RandomU32(void);
void ScopedLockCtor(ScopedLock* l);                             // 0x801F5384
void ScopedLockDtor(ScopedLock* l, s32 flags);                  // 0x801F53B0
void* ScreenSetFindElement(void* set, u32 hash);                // 0x80198F18
void* ScreenSetFindWidget(void* set, u32 hash);                 // 0x80198E34
void ScreenSetsActivate(void* sets, u32 hash);                  // 0x8018CD8C
void* ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
u32 StringHash(const char* s);                                  // 0x801EB704
s32 SysSprintf(char* buf, const char* fmt, ...);                // 0x80142EFC
void UISetText(void* el, const char* text, s32 a);              // 0x8019EFC8
s32 UISetTexture(u8* el, s32 tex);                              // 0x801A11FC
void* UIWidgetFindChild(void* widget, u32 hash);                // 0x80199AEC
s32 WorldGetPhase(void* world);                                 // 0x800D83A8

void PartInit(u8* p);                                           // 0x8021D320
void PartDestroy(u8* p, s32 del);                               // 0x8021D374
void fn_8001AFCC(u8* p);                                         // 0x8001AFCC
u8* fn_80023884(u8* p);                                         // 0x80023884
u8* fn_80025A28(u8* p);                                         // 0x80025A28
u8* fn_80028038(u8* p);                                         // 0x80028038
u8* fn_8002C83C(u8* p);                                         // 0x8002C83C
u8* fn_8002DE9C(u8* p);                                         // 0x8002DE9C
u8* fn_800314F8(u8* p);                                         // 0x800314F8
u8* fn_80034084(u8* p);                                         // 0x80034084
u8* fn_80037324(u8* p);                                         // 0x80037324
u8* fn_80038618(u8* p);                                         // 0x80038618
u8* fn_80039CF8(u8* p);                                         // 0x80039CF8
void fn_8003E0A4(void);                                         // 0x8003E0A4
u8* fn_80040824(u8* p);                                         // 0x80040824
void fn_80040ACC(u8* arg0, u8* arg1, u8* arg2);                         // 0x80040ACC
u8* fn_800497F4(u8* p);                                         // 0x800497F4
s32 fn_800508D0(void);                                          // 0x800508D0
void fn_800508EC(void);                                         // 0x800508EC
void fn_80050EB0(f32 a0);                                       // 0x80050EB0
u8* fn_8004EDD0(u8* obj, s32 flag, s32 a2);                     // 0x8004EDD0
u8* fn_8005CA88(u8* p);                                         // 0x8005CA88
void* fn_800E60BC(void);                                        // 0x800E60BC
void* fn_8011ECCC(s8 arg0);                                          // 0x8011ECCC
void fn_801265F4(void);                                         // 0x801265F4
void fn_80126808(void);                                         // 0x80126808
void fn_801458B0(f32* quat, f32* a, f32* b);                    // 0x801458B0
s32 fn_80146A7C(u8* p, const char* path);                       // 0x80146A7C
s32 fn_80146DD8(u8* p, s32 mode, s32 val);                      // 0x80146DD8
void* fn_80198F1C(void* set, u32 hash);                         // 0x80198F1C
s32 fn_8019DB14(void* el);                                      // 0x8019DB14
u32 fn_8019DC10(void* el);                                      // 0x8019DC10
s32 fn_8019DC48(void* el);                                      // 0x8019DC48
s32 fn_8019DC50(void* el);                                      // 0x8019DC50
u8* fn_801B3308(void);                                          // 0x801B3308
void fn_801B3310(u8* p, u8* q);                                 // 0x801B3310
void* fn_801D40E0(s32 n);                                       // 0x801D40E0
u8* fn_8020EAD4(u8* arg0);                                      // 0x8020EAD4
void fn_8020F2A4(u8* p);                                         // 0x8020F2A4
s32 fn_80213130(u8* battle, u8* x);                             // 0x80213130
s32 fn_80213150(u8* battle, u8* x);                             // 0x80213150
s32 fn_802188B8(void* lab, s32 idx);                            // 0x802188B8
void fn_8032F1E8(void* arr, void (*ctor)(void), void (*dtor)(void), s32 size, s32 count);
void* fn_80333D50(const char* path, const char* mode);          // 0x80333D50 (fopen)
s32 fn_80334290(void* obj);                                     // 0x80334290
void fn_80334458(void* obj, s32 x, s32 y);                      // 0x80334458
char* fn_80338BC0(char* s, const char* sub);                    // 0x80338BC0 (strstr)

extern u8 gAudio[];
extern u8 gGame[];
extern u8 gWorldState[];
extern u8 lbl_80347A78[];
extern u8 lbl_80347E10[];
extern const char lbl_80355530[];
extern const char lbl_80357DF8[];
extern s32 lbl_803582E0[];
extern u8 lbl_803598A0[];
extern u8 lbl_803C4298[];
extern s32 lbl_803C4E28[];
extern f32 lbl_805F8738;
extern const char lbl_805F873C[];
extern u8 lbl_805F91E0[];
extern const char lbl_805F91E8[];
extern f32 lbl_805F9408;
extern u8 lbl_805F95F8[];
extern u8* lbl_805FB010;
extern u8* lbl_805FB014;
extern u8* lbl_805FB01C;
extern u8* lbl_805FB020;
extern u8 lbl_805FB024;
extern u8 lbl_805FB025;
extern u8 lbl_805FB030[];
extern s32 lbl_805FB038;
extern u8* lbl_805FB0DC;
extern u8 lbl_805FB0E0;
extern u8 lbl_805FB0E2;
extern u8 lbl_805FB0E3[];
extern u8 lbl_805FB0E4;
extern s32 lbl_805FB0E8;
extern s32 lbl_805FB0EC;
extern u32 lbl_805FB0F0;
extern u8* lbl_805FB0F8;
extern u8* lbl_805FB0FC;
extern u8* lbl_805FB134;
extern f32 lbl_803475E0[];
extern f32 lbl_805FEB0C;
}
extern "C++" void CharacterSetMode(Character* c, u32 hash);  // 0x80016284
extern "C++" void* gLab;                                        // 0x805FBBC8
extern "C++" void* gScreenSets;                                 // 0x805FB654
extern "C++" u8* gBattle;                                       // 0x805FBBB8
extern "C++" u8* gSaveSystem;                                   // 0x805FBBE0
extern "C++" s32 gZoneResourceSize;                             // 0x805FB94C

extern "C" {

// 0x80146808
// Opens a file (path, mode) through the stdio wrapper and stores the handle at +4 and its
// size at +8; in the alternate mode (global == 1, mode lacks 'w') it uses the sub-reader at +0xC.
s32 fn_80146808(u8* p, const char* path, const char* mode) {
    s32 kind = *(s32*)lbl_805F91E0;
    s32 size;
    *(s32*)(p + 0x0) = kind;
    *(void**)(p + 0x4) = NULL;
    if (kind == 1 && fn_80338BC0((char*)mode, lbl_805F91E8) == NULL) {
        return fn_80146A7C(p + 0xC, path);
    }
    void* fp = fn_80333D50(path, mode);
    *(void**)(p + 0x4) = fp;
    if (fp == NULL) {
        return 0;
    }
    if (fp != NULL) {
        fn_80334458(fp, 0, 2);
        size = fn_80334290(*(void**)(p + 0x4));
    } else if (*(s32*)(p + 0x0) == 1) {
        size = fn_80146DD8(p + 0xC, 2, 0);
    } else {
        size = 0;
    }
    void* fp2 = *(void**)(p + 0x4);
    *(s32*)(p + 0x8) = size;
    if (fp2 != NULL) {
        fn_80334458(fp2, 0, 0);
        fn_80334290(*(void**)(p + 0x4));
    } else if (*(s32*)(p + 0x0) == 1) {
        fn_80146DD8(p + 0xC, 0, 0);
    }
    return 1;
}

// Shared tail of the conversation intro cases: when the world is in phase 2, clears the
// five fighter slots' state, un-flags the current object and the game-state flag bit.
static void ConversationIntroResetFighters(void) {
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        u8* g = *(u8**)(gGame + 0xD614);
        if (*(u32*)(*(u8**)(g + 0x4) + 0x1C) & 0x2000) {
            u8* slot = g;
            for (s32 i = 0; i < 5; i++) {
                u8* f = *(u8**)(slot + 0xC50);
                if (f != NULL) {
                    fn_8004EDD0(f, 0, 0);
                }
                slot += 4;
            }
        }
        if ((*(u8*(**)(u8*))(*(u8**)g + 0xC))(g) != NULL) {
            u8* o = (*(u8*(**)(u8*))(*(u8**)g + 0xC))(g);
            *(u32*)(o + 0x8) &= 0xFFFFFFFD;
        }
        u8* st = *(u8**)(g + 0x4);
        *(u32*)(st + 0x20) &= 0xFFFFFFFD;
    }
}

// 0x800F7938
// Plays the intro for a conversation type: cases 0/1 reset the fighters and call
// fn_80050EB0, case 2/4/default call fn_800508EC, case 3 sets the portrait texture of the
// conversation screen from a (key -> row) table, case 5 does nothing.
void Wii_PlayConversationIntro(BOOL a0) {
    switch (a0) {
    case 0:
    case 1:
        ConversationIntroResetFighters();
        fn_80050EB0(lbl_805FEB0C);
        break;
    case 2:
    case 4:
        ConversationIntroResetFighters();
        fn_800508EC();
        break;
    case 3: {
        s32 tbl[22];
        memcpy(tbl, lbl_80347E10, 0x58);
        s32 row = 0xD;
        for (s32 i = 0; i < 0xB;) {
            if ((u32)tbl[i * 2] == (u32)fn_800508D0()) {
                row = tbl[i * 2 + 1];
                break;
            }
            i++;
        }
        u8* ent = *(u8**)(gGameDb->data + 0x8930) + row * 8;
        s32 tex = (s32)(intptr_t)fn_8011ECCC(*(s8*)(ent + 0x4));  // lint:ok 32-bit slot
        void* set = ScreenSetsCurrent(gScreenSets);
        if (set != NULL) {
            void* w = ScreenSetFindWidget(set, 0xBEF77BAEu);
            if (w != NULL) {
                void* c = UIWidgetFindChild(w, 0x5881687Au);
                if (c != NULL) {
                    UISetTexture((u8*)c, tex);
                }
            }
            UIEvent_SendToScreen(0x240C8EE2u, 0xBEF77BAEu);
        }
        break;
    }
    case 5:
        break;
    default:
        fn_800508EC();
        break;
    }
    u8* o = (u8*)fn_800E60BC();
    if (o != NULL) {
        *(u8*)(o + 0x54) = 0;
    }
}

// 0x8001B20C
// Constructor for the lab character record: clears fields, builds the 5 parts, resets the
// zone-object pointers (+0x1A8, +0x34C, +0x4F0, +0x694, +0x838) and slot ids.
u8* fn_8001B20C(u8* p) {
    *(s32*)(p + 0x0) = 0;
    *(u8*)(p + 0x4) = 0;
    fn_8032F1E8(p + 0x83C, (void (*)(void))PartInit, (void (*)(void))PartDestroy, 0x30, 5);
    fn_8020EAD4(p + 0x940);
    *(s32*)(p + 0xA7C) = 0;
    *(s32*)(p + 0xA80) = -1;
    *(s32*)(p + 0xA84) = -1;
    *(s32*)(p + 0xA88) = -1;
    *(u8*)(p + 0xA8C) = 0;
    *(s32*)(p + 0xA90) = 0;
    *(s32*)(p + 0xAD4) = 0;
    *(s32*)(p + 0xAD8) = -1;
    *(s32*)(p + 0xADC) = 0;
    *(s32*)(p + 0xAE0) = 0;
    *(s32*)(p + 0xAE4) = 0;
    *(s32*)(p + 0xBF8) = 0;
    *(u8*)(p + 0xBFC) = 0;
    *(s32*)(p + 0xC00) = 0;
    *(u8*)(p + 0xC18) = 1;
    *(u8*)(p + 0xC19) = 1;
    *(s32*)(p + 0xC44) = 0;
    *(s32*)(p + 0x1A4) = 0;
    *(s32*)(p + 0x1A8) = 0;
    *(s32*)(p + 0x92C) = 0;
    *(s32*)(p + 0xC04) = 0;
    *(s32*)(p + 0x348) = 0;
    *(s32*)(p + 0x34C) = 0;
    *(s32*)(p + 0x930) = 0;
    *(s32*)(p + 0xC08) = 0;
    *(s32*)(p + 0x4EC) = 0;
    *(s32*)(p + 0x4F0) = 0;
    *(s32*)(p + 0x934) = 0;
    *(s32*)(p + 0xC0C) = 0;
    *(s32*)(p + 0x690) = 0;
    *(s32*)(p + 0x694) = 0;
    *(s32*)(p + 0x938) = 0;
    *(s32*)(p + 0xC10) = 0;
    *(s32*)(p + 0x834) = 0;
    *(s32*)(p + 0x838) = 0;
    *(s32*)(p + 0x93C) = 0;
    *(s32*)(p + 0xC14) = 0;
    return p;
}

// 0x8001BA70
// Finds the lab character, then scans the zone's object list for flagged objects (bit 0x40)
// whose name matches one of five known names and stores them in the record.
void* fn_8001BA70(u8* p) {
    void* ch = CharacterFind(0x4CF9FE5B);
    s32 off = 0;
    for (s32 i = 0;; i++) {
        u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
        if (i >= *(s32*)(zone + 0x98)) {
            break;
        }
        u8* o = *(u8**)(*(u8**)(zone + 0x12C) + off);
        if (*(u32*)(*(u8**)(o + 0x4) + 0x1C) & 0x40) {
            if (strcmp((char*)(o + 0x108), lbl_80355530) == 0) {
                *(u8**)(p + 0x1A8) = o;
            } else if (strcmp((char*)(o + 0x108), lbl_80355530 + 0x10) == 0) {
                *(u8**)(p + 0x4F0) = o;
            } else if (strcmp((char*)(o + 0x108), lbl_80355530 + 0x24) == 0) {
                *(u8**)(p + 0x694) = o;
            } else if (strcmp((char*)(o + 0x108), lbl_80355530 + 0x38) == 0) {
                *(u8**)(p + 0x838) = o;
            } else if (strcmp((char*)(o + 0x108), lbl_80355530 + 0x50) == 0) {
                *(u8**)(p + 0x34C) = o;
            }
        }
        off += 4;
    }
    return ch;
}

// Allocates and constructs a lab experiment object, installs it in lbl_805FB134 and switches
// the save system's game state to `state` (remembering the previous one).
static void LabStartExperiment(s32 size, u8* (*ctor)(u8*), s32 state) {
    u8* obj = (u8*)fn_801D40E0(size);
    if (obj != NULL) {
        obj = ctor(obj);
    }
    lbl_805FB134 = obj;
    u8* s = gSaveSystem;
    s32 old = *(s32*)(s + 0x108);
    *(s32*)(s + 0x10C) = old;
    *(s32*)(s + 0x108) = state;
    *(s32*)(s + 0x104) = -1;
    if (old == -1) {
        *(s32*)(s + 0x10C) = *(s32*)(s + 0x108);
    }
}

// 0x800217B0
// Starts the lab experiment selected by the screen hash (only in world phase 4): pauses the
// audio, builds the experiment object and tells the lab which index (0-9, -1 = none) started.
void fn_800217B0(u32 hash) {
    s32 idx = -1;
    if (WorldGetPhase(gGame + 0xD5F0) == 4) {
        AudioSetPaused(gAudio, 1);
        switch ((s32)hash) {
        case 0x6DBCBC22:
            idx = 0;
            LabStartExperiment(0xB14, fn_80023884, 0x13);
            break;
        case 0x7B794FF5:
            idx = 2;
            LabStartExperiment(0x230, fn_80025A28, 0x15);
            break;
        case (s32)0xA29FFAEA:
            idx = 1;
            LabStartExperiment(0x124, fn_80028038, 0x14);
            break;
        case 0x6D0FCFD5:
            idx = 3;
            LabStartExperiment(0xE8, fn_800314F8, 0x16);
            break;
        case (s32)0x92AE284A:
            idx = 7;
            LabStartExperiment(0x648, fn_80034084, 0x1A);
            break;
        case (s32)0xDA5AD4E1:
            idx = 4;
            LabStartExperiment(0x6E8, fn_80038618, 0x17);
            break;
        case 0x68BAEC35:
            idx = 5;
            LabStartExperiment(0x9C8, fn_80039CF8, 0x1B);
            break;
        case 0x49D29DC3:
            idx = 8;
            LabStartExperiment(0x3D8, fn_8002C83C, 0x18);
            *(s32*)(gSaveSystem + 0xA1C) = 0;
            break;
        case 0x48012811:
            idx = 6;
            LabStartExperiment(0xF88, fn_8002DE9C, 0x1C);
            break;
        case 0x6E59E316:
            idx = 9;
            LabStartExperiment(0xA78, fn_80037324, 0x2B);
            break;
        }
    }
    fn_802188B8(gLab, idx);
}

// Calls virtual slot 0x2C of the object's controller (the object at +0x138) with the object.
static void ObjControllerCall(u8* o, s32 arg) {
    (*(VFnI*)(*(u8**)(o + 0x138) + 0x2C))(o, arg);
}

// 0x80027434
// Lays out the 8 pickup/target records (stride 0x48 at +0xB0): grid position and
// size per record with a random initial angle, pushes the values into the linked object, then
// repositions 4 random free records' objects to their targets.
void fn_80027434(u8* p) {
    s32 off = 0;
    f32 step = (520.0f - 120.0f) / (f32)5;
    for (s32 i = 0; i < 8; i++) {
        u8* rec = *(u8**)(p + 0xB0) + off;
        u8* obj = *(u8**)(rec + 0x0);
        s32 row = i % 4;
        f32 x = 0.0f;
        if (i < 4) {
            x += 105.0f;
            if (row == 3) x += 45.0f;
        } else {
            x += 545.0f;
            if (row == 3) x -= 45.0f;
        }
        *(f32*)(rec + 0x1C) = x;
        *(f32*)(*(u8**)(p + 0xB0) + off + 0x20) = 120.0f + step * (f32)row;
        *(f32*)(*(u8**)(p + 0xB0) + off + 0x24) = 5.0f + *(f32*)(obj + 0x70);
        *(f32*)(*(u8**)(p + 0xB0) + off + 0x28) = 50.0f;
        *(f32*)(*(u8**)(p + 0xB0) + off + 0x2C) = step;
        *(f32*)(*(u8**)(p + 0xB0) + off + 0x30) = 0.0f;
        *(f32*)(*(u8**)(p + 0xB0) + off + 0x34) =
            0.017453292f * (82.0f * (-1.0f + (2.0f * (2.3283064e-10f * (f32)RandomU32()))));
        rec = *(u8**)(p + 0xB0) + off;
        obj = *(u8**)rec;
        *(f32*)(rec + 0x10) = *(f32*)(obj + 0x74);
        *(f32*)(rec + 0x14) = *(f32*)(obj + 0x78);
        *(f32*)(rec + 0x18) = *(f32*)(obj + 0x7C);
        rec = *(u8**)(p + 0xB0) + off;
        f32 hx = *(f32*)(rec + 0x10) * 0.5f;
        f32 hy = *(f32*)(rec + 0x14) * 0.5f;
        f32 hz = *(f32*)(rec + 0x18) * 0.5f;
        *(f32*)(rec + 0x38) = hx;
        *(f32*)(rec + 0x3C) = hy;
        *(f32*)(rec + 0x40) = hz;
        rec = *(u8**)(p + 0xB0) + off;
        obj = *(u8**)rec;
        *(f32*)(obj + 0x74) = *(f32*)(rec + 0x38);
        *(f32*)(obj + 0x78) = *(f32*)(rec + 0x3C);
        *(f32*)(obj + 0x7C) = *(f32*)(rec + 0x40);
        rec = *(u8**)(p + 0xB0) + off;
        *(f32*)(*(u8**)rec + 0x68) = *(f32*)(rec + 0x1C);
        *(f32*)(*(u8**)rec + 0x6C) = *(f32*)(rec + 0x20);
        *(f32*)(*(u8**)rec + 0x70) = *(f32*)(rec + 0x24);
        *(f32*)(*(u8**)rec + 0x98) = *(f32*)(rec + 0x34);
        ObjControllerCall(*(u8**)rec, 0x18);
        ObjControllerCall(*(u8**)(*(u8**)(p + 0xB0) + off), 0x18);
        ObjControllerCall(*(u8**)(*(u8**)(p + 0xB0) + off), 2);
        off += 0x48;
    }
    RandomU32();
    for (s32 n = 0; n < 4; n++) {
        u8* rec = NULL;
        while (rec == NULL) {
            s32 a = 0, b = 0, c = 0;
            rec = *(u8**)(p + 0xB0) + (RandomU32() & 7) * 0x48;
            u8* obj = *(u8**)rec;
            if (*(u8*)(rec + 0x44) == 0 && fabsf(*(f32*)(obj + 0x68) - *(f32*)(rec + 0x4)) < 40.0f) {
                c = 1;
            }
            if (c != 0 && fabsf(*(f32*)(*(u8**)rec + 0x6C) - *(f32*)(rec + 0x8)) < 40.0f) {
                b = 1;
            }
            if (b != 0 && fabsf(*(f32*)(*(u8**)rec + 0x98)) < 0.3f) {
                a = 1;
            }
            if (a != 0 || *(u8*)(rec + 0x44) != 0) {
                rec = NULL;
            }
        }
        *(f32*)(*(u8**)rec + 0x68) = *(f32*)(rec + 0x4);
        *(f32*)(*(u8**)rec + 0x6C) = *(f32*)(rec + 0x8);
        *(f32*)(*(u8**)rec + 0x70) = *(f32*)(rec + 0xC);
        *(f32*)(*(u8**)rec + 0x98) = 0.0f;
        ObjControllerCall(*(u8**)rec, 0x18);
        f32 vx = *(f32*)(rec + 0x10);
        f32 vy = *(f32*)(rec + 0x14);
        f32 vz = *(f32*)(rec + 0x18);
        *(f32*)(*(u8**)rec + 0x74) = vx;
        *(f32*)(*(u8**)rec + 0x78) = vy;
        *(f32*)(*(u8**)rec + 0x7C) = vz;
    }
}

// 0x8003451C
// Sets up the Gem-catch minigame screen: clears state, writes the tuning constants, looks up
// the gem/basket/background UI elements by name hash and stores them in the record.
void fn_8003451C(u8* p) {
    char buf[0x20];
    memset(p + 0x1CC, 0, 0x78);
    memset(p + 0x2DC, 0, 0x78);
    memset(p + 0x39C, 0, 0x78);
    memset(p + 0x45C, 0, 0x78);
    memset(p + 0x5A0, 0, 0xC);
    memset(p + 0x18C, 0, 0xC);
    memset(p + 0x244, 0, 1);
    memset(p + 0x354, 0, 1);
    memset(p + 0x414, 0, 1);
    memset(p + 0x4D4, 0, 1);
    *(f32*)(p + 0x170) = 10.0f;
    *(f32*)(p + 0x174) = 0.125f;
    *(f32*)(p + 0x184) = 0.125f;
    *(f32*)(p + 0x188) = 150.0f;
    *(f32*)(p + 0x594) = 300.0f;
    *(f32*)(p + 0x590) = 600.0f;
    *(f32*)(p + 0x5B0) = 1.0f;
    *(f32*)(p + 0x5B8) = 2000.0f;
    *(f32*)(p + 0x5C0) = 0.5235988f;
    *(f32*)(p + 0x5C4) = 8.0f;
    *(f32*)(p + 0x5BC) = 1.3962634f;
    *(f32*)(p + 0x18C) = 75.0f;
    *(f32*)(p + 0x190) = -75.0f;
    *(f32*)(p + 0x194) = 0.0f;
    *(f32*)(p + 0x178) = 120.0f;
    *(f32*)(p + 0x5CC) = 300.0f;
    *(f32*)(p + 0x5D0) = 344.0f;
    *(f32*)(p + 0x5C8) = 410.0f;
    *(f32*)(p + 0x5D4) = 40.0f;
    *(f32*)(p + 0x5D8) = 54.0f;
    *(f32*)(p + 0x5DC) = 50.0f;
    *(f32*)(p + 0x5E0) = 128.0f;
    *(f32*)(p + 0x5E4) = 73.0f;
    *(f32*)(p + 0x5E8) = 96.0f;
    *(s32*)(p + 0x5F4) = 9;
    *(f32*)(p + 0x144) = 1.0f;
    *(s32*)(p + 0x128) = 0;
    if (*(u8*)(p + 0x644) == 0) {
        fn_801B3310(fn_801B3308(), p + 0x90);
        *(u8*)(p + 0x644) = 1;
    }
    *(u8*)(p + 0x168) = 0;
    void* set = ScreenSetsCurrent(gScreenSets);
    for (s32 i = 0; i < 10; i++) {
        SysSprintf(buf, "Gem_%d", i);
        *(void**)(p + 0x198 + i * 4) = ScreenSetFindElement(set, StringHash(buf));
        *(u8*)(p + 0x1C0 + i) = 0;
        SysSprintf(buf, lbl_80357DF8, i);
        *(void**)(p + 0x2A8 + i * 4) = ScreenSetFindElement(set, StringHash(buf));
        *(u8*)(p + 0x2D0 + i) = 0;
        SysSprintf(buf, lbl_80357DF8 + 0x10, i);
        *(void**)(p + 0x368 + i * 4) = ScreenSetFindElement(set, StringHash(buf));
        *(u8*)(p + 0x390 + i) = 0;
        SysSprintf(buf, lbl_80357DF8 + 0x1C, i);
        *(void**)(p + 0x428 + i * 4) = ScreenSetFindElement(set, StringHash(buf));
        *(u8*)(p + 0x450 + i) = 0;
    }
    for (s32 i = 0; i < 0x28; i++) {
        SysSprintf(buf, lbl_80357DF8 + 0x28, i);
        *(void**)(p + 0x4E8 + i * 4) = ScreenSetFindElement(set, StringHash(buf));
    }
    *(void**)(p + 0x59C) = ScreenSetFindElement(set, StringHash("Basket"));
    *(void**)(p + 0x604) = ScreenSetFindElement(set, StringHash(lbl_80357DF8 + 0x34));
    *(void**)(p + 0x608) = ScreenSetFindElement(set, StringHash(lbl_80357DF8 + 0x44));
    *(void**)(p + 0x628) = fn_80198F1C(set, StringHash(lbl_80357DF8 + 0x54));
    *(void**)(p + 0x160) = ScreenSetFindElement(set, StringHash(lbl_80357DF8 + 0x60));
    f32 zero = 0.0f;
    s32 a = 0, b = 0;
    if (*(f32*)(p + 0x5F8) == zero && *(f32*)(p + 0x5FC) == zero) {
        a = 1;
    }
    if (a != 0 && *(f32*)(p + 0x600) == zero) {
        b = 1;
    }
    if (b != 0) {
        u8* bk = *(u8**)(p + 0x160);
        *(f32*)(p + 0x5F8) = *(f32*)(bk + 0x68);
        *(f32*)(p + 0x5FC) = *(f32*)(bk + 0x6C);
        *(f32*)(p + 0x600) = *(f32*)(bk + 0x70);
        *(f32*)(p + 0x164) = *(f32*)(bk + 0x74);
    }
    *(f32*)(*(u8**)(p + 0x160) + 0x74) = 0.0f;
    *(f32*)(*(u8**)(p + 0x160) + 0x68) = 700.0f;
    ObjControllerCall(*(u8**)(p + 0x160), 0x18);
    memset(p + 0x14C, 0, 5);
    *(void**)(p + 0x14C) = ScreenSetFindElement(set, 0x40E7BCB4u);
    *(void**)(p + 0x150) = ScreenSetFindElement(set, 0x8D1E0C17u);
    *(void**)(p + 0x154) = ScreenSetFindElement(set, 0x3F2ED7A1u);
    *(void**)(p + 0x158) = ScreenSetFindElement(set, 0x4FE9409Au);
    *(void**)(p + 0x15C) = ScreenSetFindElement(set, 0x00FB4B78u);
    *(s32*)(p + 0x17C) = 2;
    ScreenSetsCurrent(gScreenSets);
    for (s32 i = 0; i < 5; i++) {
        u8* e = *(u8**)(p + 0x14C + i * 4);
        if (i == 2) {
            (*(VFnI*)(*(u8**)(e + 0x138) + 0x2C))(e, 2);
        } else {
            (*(VFnI*)(*(u8**)(e + 0x138) + 0x30))(e, 2);
        }
    }
}

// Marks the object's transform (at +4) dirty: ORs the four update flags into +0x20.
static void ObjTransformDirty(u8* o) {
    *(u32*)(*(u8**)(o + 0x4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(o + 0x4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(o + 0x4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(o + 0x4) + 0x20) |= 0x10000000;
}

// 0x8003CF7C
// Per-frame update of the two-fighter camera object: (once all slots are filled and the
// scene is active) on the first frame builds the camera/battle objects, aims the camera at the
// midpoint of the two fighters, then ticks the camera object with dt.
void fn_8003CF7C(f32 dt) {
    if (lbl_805FB025 == 0) {
        return;
    }
    for (s32 i = 0; i < lbl_805FB038; i++) {
        if (lbl_803C4E28[i] == 0) {
            return;
        }
    }
    if (lbl_805FB024 == 0) {
        fn_8003E0A4();
        u8* g = *(u8**)(gGame + 0xD614);
        if (*(u32*)(g + 0x2C) == 0xC595F858u) {
            ScopedLock lock;
            ScopedLockCtor(&lock);
            (*(VFnI*)(*(u8**)g + 0x18))(g, 1);
            ScopedLockDtor(&lock, -1);
        }
        if (lbl_803C4298[0x1C] != 0x1C) {
            u8* obj = (u8*)fn_801D40E0(0x4F8);
            if (obj != NULL) {
                obj = fn_80040824(obj);
            }
            lbl_805FB014 = obj;
            fn_80040ACC(obj, lbl_805FB01C, lbl_805FB020);
        } else if (gBattle != NULL) {
            u8* a = (u8*)fn_801D40E0(0x48);
            u8* a2 = a;
            if (a != NULL) {
                a2 = fn_800497F4(a);
            }
            fn_80213130(gBattle, a2);
            u8* b = (u8*)fn_801D40E0(4);
            u8* b2 = b;
            if (b != NULL) {
                fn_8020F2A4(b);
                b2 = b;
            }
            fn_80213150(gBattle, b2);
        }
        u8* f1 = *(u8**)(lbl_805FB01C + 0x1C);
        if (f1 != NULL) f1 -= 0xC48;
        (*(VFnIF*)(*(u8**)f1 + 0x20))(f1, 0, 0.0f);
        u8* f2 = *(u8**)(lbl_805FB020 + 0x1C);
        if (f2 != NULL) f2 -= 0xC48;
        (*(VFnIF*)(*(u8**)f2 + 0x20))(f2, 0, 0.0f);

        u8* o2 = *(u8**)(lbl_805FB020 + 0x1C);
        if (o2 != NULL) o2 -= 0xC48;
        u8* pos2 = *(u8**)(o2 + 0x4);
        u8* o1 = *(u8**)(lbl_805FB01C + 0x1C);
        f32 x2 = *(f32*)(pos2 + 0x0);
        f32 y2 = *(f32*)(pos2 + 0x4);
        f32 z2 = *(f32*)(pos2 + 0x8);
        if (o1 != NULL) o1 -= 0xC48;
        u8* pos1 = *(u8**)(o1 + 0x4);
        f32 x1 = *(f32*)(pos1 + 0x0);
        f32 y1 = *(f32*)(pos1 + 0x4);
        f32 z1 = *(f32*)(pos1 + 0x8);
        f32 dx = x2 - x1;
        f32 dy = y2 - y1;
        f32 dz = z2 - z1;
        f32 len2 = dz * dz + (dx * dx + dy * dy);
        f32 dir[3];
        if (len2 == 0.0f) {
            dir[0] = 0.0f;
            dir[1] = 0.0f;
            dir[2] = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
            dir[0] = dx * inv;
            dir[1] = dy * inv;
            dir[2] = dz * inv;
        }
        f32 mx = (x2 + x1) / 2.0f;
        f32 my = (y2 + y1) / 2.0f;
        f32 mz = (z2 + z1) / 2.0f;
        u8* cam = *(u8**)(lbl_805FB014 + 0x4D4);
        u8* t = *(u8**)(cam + 0x4);
        *(f32*)(t + 0x0) = mx;
        *(f32*)(t + 0x4) = my;
        *(f32*)(t + 0x8) = mz;
        ObjTransformDirty(cam);
        f32 up[3];
        up[0] = 0.0f;
        up[1] = 0.0f;
        up[2] = 1.0f;
        f32 quat[4];
        fn_801458B0(quat, dir, up);
        cam = *(u8**)(lbl_805FB014 + 0x4D4);
        t = *(u8**)(cam + 0x4);
        *(f32*)(t + 0xC) = quat[0];
        *(f32*)(t + 0x10) = quat[1];
        *(f32*)(t + 0x14) = quat[2];
        *(f32*)(t + 0x18) = quat[3];
        ObjTransformDirty(cam);
        if (lbl_805FB030[0] != 0) {
            for (s32 i = 0; i < 6; i++) {
                if (lbl_803C4298[0x1C] == lbl_803582E0[i]) {
                    f32 k = lbl_803475E0[i];
                    f32 ox = dir[0] * k;
                    f32 oy = dir[1] * k;
                    f32 oz = dir[2] * k;
                    f32 tx = x2 + ox;
                    f32 ty = y2 + oy;
                    f32 tz = z2 + oz;
                    u8* o = *(u8**)(lbl_805FB020 + 0x1C);
                    if (o != NULL) o -= 0xC48;
                    u8* tp = *(u8**)(o + 0x4);
                    *(f32*)(tp + 0x0) = tx;
                    *(f32*)(tp + 0x4) = ty;
                    *(f32*)(tp + 0x8) = tz;
                    ObjTransformDirty(o);
                    break;
                }
            }
        }
    }
    if (lbl_805FB010 != NULL) {
        (*(VFnF*)(*(u8**)lbl_805FB010 + 0xC))(lbl_805FB010, dt);
    }
}

// Rotates the vector (a, 0, 0) by the quaternion stored at t+0xC (x, y, z, w), result in out.
static void TransformRotateAxisX(u8* t, f32 a, f32* out) {
    f32 qx = *(f32*)(t + 0xC);
    f32 qy = *(f32*)(t + 0x10);
    f32 qz = *(f32*)(t + 0x14);
    f32 qw = *(f32*)(t + 0x18);
    f32 zero = 0.0f;
    f32 w0 = qw * zero;
    f32 x0 = qx * zero;
    f32 y0 = qy * zero;
    f32 z0 = qz * zero;
    f32 tx = (y0 + ((qw * a) + x0)) - z0;
    f32 tw = ((w0 - (qx * a)) - y0) - z0;
    f32 ty = (qz * a) + (y0 + (w0 - x0));
    f32 tz = z0 + ((w0 + x0) - (qy * a));
    f32 cx = -qx;
    f32 cy = -qy;
    f32 cz = -qz;
    out[0] = ((ty * cz) + ((tw * cx) + (tx * qw))) - (tz * cy);
    out[1] = (tz * cx) + ((ty * qw) + ((tw * cy) - (tx * cz)));
    out[2] = (tz * qw) + (((tw * cz) + (tx * cy)) - (ty * cx));
}

// 0x800508EC
// Camera shot for the conversation: looks up the shot offsets (table keyed by the object's
// record id, 16 entries of key + 5 floats) and aims the camera at the lab object: eye = object
// position + its Y axis (rotated by the offset) * distance, target = object position shifted.
void fn_800508EC(void) {
    struct ShotEntry {
        u32 key;
        f32 v[5];
    };
    ShotEntry tbl[16];
    memcpy(tbl, lbl_80347A78, 0x180);
    u8* obj = lbl_805FB0DC;
    if (obj == NULL) {
        return;
    }
    u8* t = *(u8**)(obj + 0x4);
    f32 pos[3];
    pos[0] = *(f32*)(t + 0x0);
    pos[1] = *(f32*)(t + 0x4);
    pos[2] = *(f32*)(t + 0x8);
    Vec3 ay = QuatGetAxisY((const Quat*)(*(u8**)(obj + 0x4) + 0xC));
    f32 v0 = tbl[0].v[0];
    f32 v1 = tbl[0].v[1];
    f32 v2 = tbl[0].v[2];
    f32 v3 = tbl[0].v[3];
    f32 v4 = tbl[0].v[4];
    for (s32 i = 0; i < 16; i++) {
        u8* rec = *(u8**)(obj + 0x9C);
        u32 key = (rec != NULL) ? *(u32*)(rec + 0x8) : 0;
        if (tbl[i].key == key) {
            v4 = tbl[i].v[4];
            v0 = tbl[i].v[0];
            v1 = tbl[i].v[1];
            v2 = tbl[i].v[2];
            v3 = tbl[i].v[3];
            break;
        }
    }
    t = *(u8**)(obj + 0x4);
    f32 r[3];
    TransformRotateAxisX(t, -v0, r);
    f32 sx = ay.x + r[0];
    f32 sy = ay.y + r[1];
    f32 sz = ay.z + r[2];
    f32 len2 = (sz * sz) + ((sx * sx) + (sy * sy));
    f32 dir[3];
    if (len2 == 0.0f) {
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
        dir[0] = sx * inv;
        dir[1] = sy * inv;
        dir[2] = sz * inv;
    }
    f32 eye[3];
    eye[0] = (dir[0] * v4) + pos[0];
    eye[1] = (dir[1] * v4) + pos[1];
    eye[2] = v1 + ((dir[2] * v4) + pos[2]);
    t = *(u8**)(obj + 0x4);
    f32 r2[3];
    TransformRotateAxisX(t, -v2, r2);
    pos[2] = v3 + (pos[2] + r2[2]);
    pos[1] = pos[1] + r2[1];
    pos[0] = pos[0] + r2[0];
    u8* cam = lbl_803C4298 + 0;
    cam = *(u8**)(lbl_803C4298 + 0x4);
    (*(VFnI*)(*(u8**)cam + 0xC))(cam, 6);
    (*(VFnPP*)(*(u8**)cam + 0x2C))(cam, eye, pos);
    (*(VFnF*)(*(u8**)cam + 0x30))(cam, 0.7f);
    fn_801265F4();
}

// 0x80051940
// Per-frame update of the conversation overlay: creates the text widget when the screen is
// available, advances/skips the dialogue page on button 1, and runs the intro-animation,
// outro-animation and pop-screen state machines.
void fn_80051940(f32 dt) {
    if (lbl_805FB0E0 != 0) {
        if (lbl_805FB0F8 == NULL) {
            void* w = ScreenSetFindWidget(ScreenSetsCurrent(gScreenSets), 0xBEF77BAEu);
            s32 ok = 0;
            if (ScreenSetFindWidget(ScreenSetsCurrent(gScreenSets), 0xBEF77BAEu) != NULL
                && *(s32*)(gSaveSystem + 0xA1C) == 0 && lbl_805FB0E4 != 0
                && WorldGetPhase(gGame + 0xD5F0) != 4) {
                ok = 1;
            }
            if (ok != 0) {
                lbl_805FB0F8 = (u8*)UIWidgetFindChild(w, 0xF8F01FECu);
                UISetText(lbl_805FB0F8, lbl_805F873C, 0);
                if (WorldGetPhase(gGame + 0xD5F0) == 2) {
                    ScreenSetsActivate(gScreenSets, 0x7A1060A7u);
                }
                if (lbl_805FB0EC != 0) {
                    HudShowConversation(*(void**)(gGame + 0xD634), lbl_805FB0EC);
                    lbl_805FB0EC = 0;
                    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
                        ExplorationPromptClear(1);
                    } else if (WorldGetPhase(gGame + 0xD5F0) == 1) {
                        fn_80126808();
                        u8* s = *(u8**)((u8*)gScreenSets + 0x88);
                        if (s != NULL) {
                            u8* w2 = (u8*)ScreenSetFindWidget(s, 0xE7667FBEu);
                            if (w2 != NULL) {
                                (*(VFnI*)(*(u8**)(w2 + 0x138) + 0x30))(w2, 2);
                            }
                        }
                    }
                }
            }
        }
        if (lbl_805FB0F8 != NULL && fn_8019DC48(lbl_805FB0F8) > 1 && WasButtonPressed(1) != 0) {
            f32 pos = *(f32*)(lbl_805FB0F8 + 0x14C);
            if (*(u32*)(lbl_805FB0F8 + 0x64) & 0x10000) {
                pos *= lbl_805F9408;
            }
            if (fn_8019DC48(lbl_805FB0F8) > 1) {
                s32 n = fn_8019DC48(lbl_805FB0F8);
                s32 last = fn_8019DC50(lbl_805FB0F8) / n;
                if (pos >= (f32)last) {
                    fn_8019DB14(lbl_805FB0F8);
                    goto sendEvent;
                }
            }
            *(f32*)(lbl_805FB0F8 + 0x14C) = 1.0f + ((f32)*(s32*)(lbl_805FB0F8 + 0x150) / lbl_805F9408);
        sendEvent:
            UIEvent_SendToElement(0xEA6C53A1u, 0xF8F01FECu);
        }
        if (lbl_805FB0E2 != 0 && UIElement_IsUIAnimPlaying(0x246D223Fu) == 0) {
            UIActivationGrid_SetEnabled(1);
            lbl_805FB0E2 = 0;
        }
        if (((lbl_805FB0F0 >> 1) & 1) && lbl_805FB0E8 != 0 && lbl_805FB0DC != NULL) {
            u8* d = lbl_805FB0DC;
            f32 len = AnimGetLength((*(void* (**)(u8*))(*(u8**)d + 0x10))(d));
            f32 t = AnimGetTime((*(void* (**)(u8*))(*(u8**)lbl_805FB0DC + 0x10))(lbl_805FB0DC));
            if (t / len >= 1.0f) {
                d = lbl_805FB0DC;
                if (*(u32*)(*(u8**)(d + 0x4) + 0x1C) & 0x10) {
                    (*(VFnI*)(*(u8**)d + 0x84))(d, lbl_805FB0E8);
                } else {
                    (*(VFnI*)(*(u8**)d + 0x88))(d, lbl_805FB0E8);
                }
                lbl_805FB0F0 &= 0xFFFFFFFD;
            }
        }
    }
    if (lbl_805F8738 >= 0.0f) {
        if (UIElement_IsUIAnimPlaying(0x24542991u) == 0) {
            lbl_805F8738 = lbl_805F8738 - dt;
        }
        if (lbl_805F8738 <= 0.0f) {
            lbl_805F8738 = -1.0f;
            UIElement_PlayUIAnim(0x24542991u, 0x9C543022u);
        }
    }
    if (lbl_805FB0E3[0] != 0 && lbl_805FB0E0 == 0) {
        if (UIElement_IsUIAnimPlaying(0x3BD6A2A1u) == 0 && lbl_805FB0FC != NULL
            && WasButtonPressed(1) != 0) {
            f32 pos = *(f32*)(lbl_805FB0FC + 0x14C);
            if (*(u32*)(lbl_805FB0FC + 0x64) & 0x10000) {
                pos *= lbl_805F9408;
            }
            if (fn_8019DC10(lbl_805FB0FC) != 0) {
                lbl_805FB0E3[0] = 0;
            } else {
                if (fn_8019DC48(lbl_805FB0FC) > 1) {
                    s32 n = fn_8019DC48(lbl_805FB0FC);
                    s32 last = fn_8019DC50(lbl_805FB0FC) / n;
                    if (pos >= (f32)last) {
                        fn_8019DB14(lbl_805FB0FC);
                        goto doneInput;
                    }
                }
                *(f32*)(lbl_805FB0FC + 0x14C) =
                    1.0f + ((f32)*(s32*)(lbl_805FB0FC + 0x150) / lbl_805F9408);
            }
        }
    doneInput:
        if (lbl_805FB0E3[0] == 0) {
            if (WorldGetPhase(gGame + 0xD5F0) == 2) {
                UnPauseAllExplorationCharacters();
            }
            UIElement_PlayUIAnim(0x3BD6A2A1u, 0xC54D0D97u);
        }
    } else if (lbl_805FB0E3[0] == 0 && lbl_805FB0FC != NULL
               && UIElement_IsUIAnimPlaying(0x3BD6A2A1u) == 0) {
        UIScreen_PopScreen(0xBEF77BAEu);
        lbl_805FB0FC = NULL;
    }
}

// 0x80052530
// Constructor of the experiment machine object: runs the base constructor, installs the
// vtable, clears state, and optionally snapshots the transform position into +0x74C.
u8* fn_80052530(u8* p, s32 a1, u8* a2, s32 a3) {
    fn_8005CA88(p);
    *(void**)(p + 0x0) = lbl_803598A0;
    *(s32*)(p + 0x87C) = 0;
    *(s32*)(p + 0x880) = 0;
    *(s32*)(p + 0x884) = 0;
    *(s32*)(p + 0x888) = 0;
    *(s32*)(p + 0x88C) = 0;
    *(s32*)(p + 0x890) = 0;
    *(u8*)(p + 0x894) = 1;
    *(u8*)(p + 0x895) = 0;
    fn_8020EAD4(p + 0x898);
    u8* t = *(u8**)(p + 0x4);
    if (t != NULL) {
        *(u32*)(t + 0x1C) |= 0x10;
        *(u8*)(*(u8**)(p + 0x4) + 0x34) = 4;
    }
    if (a2 != NULL) {
        t = *(u8**)(p + 0x4);
        *(f32*)(p + 0x74C) = *(f32*)(t + 0x0);
        *(f32*)(p + 0x750) = *(f32*)(t + 0x4);
        *(f32*)(p + 0x754) = *(f32*)(t + 0x8);
        if (a3 != 0) {
            *(f32*)(t + 0x24) = *(f32*)(p + 0x5F4);
        }
    }
    if (*(s32*)lbl_805F95F8 == -1) {
        CharacterSetMode((Character*)p, *(u32*)(*(u8**)(p + 0x9C) + 0x8));
        fn_8001AFCC(p + 0x170);
    }
    *(s32*)(p + 0x830) = -1;
    return p;
}

}  // extern "C"
