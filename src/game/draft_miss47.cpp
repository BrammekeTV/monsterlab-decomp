// Missing functions: UI/lab helpers in 0x8012xxxx-0x8014xxxx, string/log record init, audio reset.
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "revolution.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void   CameraSetMatrix(void* camera, const f32 mtx[3][4]);          // 0x801D6814
void*  CharacterFind(s32 id);                                       // 0x800E876C
void*  EventPost(s32 id, void* sender);                             // 0x801F3918
u8*    GetGlobalSettings(void);                                     // 0x801D2BF8
s32    PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);      // 0x80209E70
s32    PlaySoundAt(void* sound, void* owner, const void* pos, int b); // 0x80209ABC
void*  ScreenSetFindElement(void* set, u32 hash);                   // 0x80198F18
void*  ScreenSetsCurrent(void* sets);                               // 0x8018D1E4
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner); // 0x801C80E0
int    SysSprintf(char* buf, const char* fmt, ...);
s32 TriggerDoAction(void* action, void* a1, void* a2);           // 0x801E0054
void*  UIWidgetFindChild(void* widget, u32 hash);                   // 0x80199AEC
s32    __cvt_fp2signed(s32 v);                                      // 0x80332ED0 (integer abs)
void   fn_8003DCE0(u8* p);
void   fn_8004C718(u8* p, s32 idx, void* elem);
void   fn_8004D760(u8* c, u8* monster);
void*  fn_8004DAF8(void* c, s32 a, s32 b);
void   fn_8004DCEC(u8* p, s32 idx);
u8*    fn_8004EDD0(u8* obj, s32 flag, s32 a2);
void*  fn_8004FFB4(void* id, void* part);
void   fn_80116EF0(u8* p, s32 a);
void   fn_80132BC0(u8* self);                                       // 0x80132BC0
BOOL   fn_80142358(void);
void   fn_8013AFF0(void);                                           // 0x8013AFF0
void   fn_80150148(void);
void   fn_801514F0(void);
void   fn_801542DC(void);
void   fn_80154318(void);
s32    fn_8019CC7C(void* el, u32 flag);
s32    fn_801A3010(void* obj, void* data);
void   fn_801BF398(u8* list, u8* item);
void*  fn_801D40E0(s32 n);
void   fn_801D66C4(f32 a, f32 b);
void   fn_801D6780(void);
void   fn_801EBA20(void* list, void* item);
void   fn_801F357C(void);
s32    fn_801F38F0(void);                                           // 0x801F38F0
void   fn_801F51DC(s32 flag);
s32    fn_801F525C(void);
BOOL   fn_801F56FC(s32 flags);
void   fn_801F5734(void* arg);
void   fn_801F57B8(void);
s32    fn_801F57E0(f32 dt);
void   fn_801FD730(u8* audio);
void   fn_801FF2A8(u8* audio, s32 handle);
void   fn_801FFC04(u8* audio, f32 v);
void fn_802000F0(u8* audio);                                      // 0x802000F0
void*  fn_80209814(void* v);
void   fn_8020EDF0(u8* dst, u8* src);
void   fn_80220A0C(s32 a);
void   OSLockMutex(u8* p);
void   OSUnlockMutex(u8* p);
s32    fn_802E9EE0(void);                                           // 0x802E9EE0
u8     fn_802E9F3C(void);                                           // 0x802E9F3C
s32    fn_802F79BC(u8 idx, u8* buf, s32 size);                      // 0x802F79BC
u64    fn_802FB094(u8* rec);
void   fclose(void* obj);
void*  UTF8ToUCS2String(const char* path, const char* mode);             // 0x80333D50 (fopen)
s32    fn_8033ADCC(s32* out);                                       // 0x8033ADCC (time)
void   operator_delete(void* p);                                    // 0x801D426C

extern u8   gAudio[];
extern u8   lbl_8047C640[];
extern u8   lbl_8047C760[];
extern u8   lbl_803C4E28[];
extern u8   lbl_803F0D10[];
extern u8   lbl_803F1068[];
extern u8   lbl_803F20C4[];
extern u8   lbl_803F2250[];
extern u8   lbl_80375E18[];
extern u8   lbl_80556AC8[];
extern u8   lbl_80556E80[];
extern u8   lbl_80559D78[];
extern s32  lbl_8055C578[];
extern s32  lbl_805F9030;
extern u32  lbl_805F9614;
extern f32  lbl_805F9970;
extern f32  lbl_805F9974;
extern f32  lbl_805F9978;
extern f32  lbl_805F997C;
extern s32  lbl_805FB034;
extern s32  lbl_805FB038;
extern s32  lbl_805FB23C;
extern s32  lbl_805FB350;
extern s32  lbl_805FB35C;
extern u8   lbl_805FB3B5;
extern u8*  lbl_805FB3C8;
extern u8*  lbl_805FB3CC;
extern void* lbl_805FB3D0;
extern s32  lbl_805FB3D4;
extern u8   lbl_805FB3DC;
extern u8   lbl_805FB3DD;
extern s32  lbl_805FB3E4;
extern s32  lbl_805FB3EC;
extern s32  lbl_805FB440;
extern u8   lbl_805FB444;
extern s32  lbl_805FB448;
extern s32  lbl_805FB44C;
extern u8   lbl_805FB454;
extern u8*  lbl_805FB488;
extern u8   lbl_805FB50C;
extern s32  lbl_805FB6B8;
extern u8*  lbl_805FB75C;
extern f32  lbl_805FB918;
extern void* lbl_805FB964;
extern void* lbl_805FBBAC;
extern u8   lbl_805FBBB3;
}
extern "C++" void* gScreenSets;                                      // 0x805FB654

typedef s32 (*VSlotFn)(void* self, s32 arg);

static const u32 kHashMenuHighlight = 0x6719867Eu;  // "ui_general_menu_highlight"
static const u32 kHashAlertIcon     = 0x207AC964u;  // "Alert_icon"
static const u32 kHashMatGoo        = 0x1B164D9Cu;  // "gOo"
static const u32 kHashMatGrass      = 0x5F595D43u;  // "grass"
static const u32 kHashMatWater      = 0x0C67D66Au;  // "water"
static const u32 kHashMatWood       = 0x0A08F4B9u;  // "wood"
static const u32 kHashMatMetal      = 0x37363679u;  // "metal"
static const u32 kHashMatTile       = 0x401A44B0u;  // "TILE"
static const u32 kHashMatDirt       = 0xAEF20C02u;  // "dirt"
static const u32 kHashMatDefault    = 0xDC1BFEC5u;  // "default"

extern "C" {

// 0x801BDA0C
// Walks a four-level tree of 6-slot node tables hanging off the world's list (arg0+4 -> +0x14),
// handing each leaf item to fn_801BF398 and freeing each table node through its owner's list.
void fn_801BDA0C(u8* arg0) {
    u8* root = *(u8**)(*(u8**)(arg0 + 0x4) + 0x14);
    u8* n = root ? root + 8 : NULL;
    while (n != NULL) {
        u8* next = *(u8**)(n - 4);
        u8* nextN = next ? next + 8 : NULL;
        u8* p24 = n;
        for (s32 i = 0; i < 6; i++) {
            u8* a = *(u8**)(p24 + 0x4);
            if (a != NULL) {
                u8* p23 = a;
                for (s32 j = 0; j < 6; j++) {
                    u8* b = *(u8**)(p23 + 0x4);
                    if (b != NULL) {
                        u8* p22 = b;
                        for (s32 k = 0; k < 6; k++) {
                            u8* c = *(u8**)(p22 + 0x4);
                            if (c != NULL) {
                                u8* p21 = c;
                                for (s32 l = 0; l < 6; l++) {
                                    u8* item = *(u8**)(p21 + 0x4);
                                    if (item != NULL) fn_801BF398(arg0, item);
                                    p21 += 4;
                                }
                                fn_801EBA20(*(void**)(*(u8**)c + 0x68), c);
                            }
                            p22 += 4;
                        }
                        fn_801EBA20(*(void**)(*(u8**)b + 0x68), b);
                    }
                    p23 += 4;
                }
                fn_801EBA20(*(void**)(*(u8**)a + 0x68), a);
            }
            p24 += 4;
        }
        fn_801EBA20(*(void**)(*(u8**)n + 0x68), n);
        n = nextN;
    }
}

// 0x8020A6C8
// Audio reset: applies the pending master volume, releases and clears all 256 voice records,
// rebuilds the 256-entry identity table, then calls the mixer reset.
void fn_8020A6C8(void) {
    f32 v = 0.0f;
    if (lbl_805F9970 >= 0.0f) {
        f32 x = lbl_805F997C;
        if (!(x < 0.0f)) {
            v = 1.0f;
            if (!(x > 1.0f)) v = x;
        }
        lbl_805F9978 = v;
        fn_801FFC04(gAudio, v);
        lbl_805F9974 = -1.0f;
        lbl_805F9970 = -1.0f;
    }
    fn_801FD730(gAudio);
    u8* rec2 = lbl_80559D78;
    lbl_805FBBB3 = 0;
    for (s32 i = 0; i < 0x100; i++) {
        u8* rec = lbl_80559D78 + (((u32)i | 0x0AB00000) & 0xFFFFF) * 0x28;
        s32 handle = *(s32*)(rec + 0x14);
        if (handle != -1) {
            fn_801FF2A8(gAudio, handle);
            *(s32*)(rec + 0x14) = -1;
        }
        *(s32*)(rec + 0x0) = 0;
        *(s32*)(rec + 0x4) = lbl_805FB6B8;
        *(f32*)(rec + 0x8) = 0.0f;
        *(f32*)(rec + 0xC) = 0.0f;
        *(f32*)(rec + 0x10) = 0.0f;
        *(s32*)(rec + 0x14) = -1;
        *(f32*)(rec + 0x18) = 0.0f;
        *(s32*)(rec + 0x24) = 0;
        *(s32*)(rec + 0x20) = 0;
        *(s32*)(rec + 0x1C) = -1;
        if (rec2 != NULL) {
            *(s32*)(rec2 + 0x0) = 0;
            *(s32*)(rec2 + 0x4) = lbl_805FB6B8;
            *(f32*)(rec2 + 0x8) = 0.0f;
            *(f32*)(rec2 + 0xC) = 0.0f;
            *(f32*)(rec2 + 0x10) = 0.0f;
            *(s32*)(rec2 + 0x14) = -1;
            *(f32*)(rec2 + 0x18) = 0.0f;
            *(s32*)(rec2 + 0x24) = 0;
            *(s32*)(rec2 + 0x20) = 0;
            *(s32*)(rec2 + 0x1C) = -1;
        }
        rec2 += 0x28;
    }
    for (s32 i = 0; i < 0x100; i++) lbl_8055C578[i] = i;
    fn_802000F0(gAudio);
}

// 0x8012112C
// Resets the main camera: identity matrix, default projection, clears a view flag, and sends two
// fixed GX register writes before and after the camera helpers.
void fn_8012112C(void) {
    fn_80150148();
    fn_801D6780();
    fn_801D66C4(0.5f, 1000.0f);
    *(s32*)(lbl_805FB75C + 0x188) = 0;
    lbl_805F9614 = 0;
    f32 m[3][4] = {{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}};
    CameraSetMatrix(lbl_8047C640, m);
    fn_801514F0();
    lbl_805FB50C &= ~8;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00C000u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0;
    fn_801542DC();
    fn_80154318();
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE000FE0u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x410004A0u;
}

// 0x80121C90
// Impact handler: maps the last hit material to a sound/effect slot, plays the sound, spawns the
// hit effect at the contact position and fires the matching trigger action.
void fn_80121C90(u8* arg0, u8* arg1, u8* arg2, f32* arg3) {
    s32 slot;
    switch (GetLastMatTypeHit(arg1)) {
    case kHashMatGoo:     slot = 1; break;
    case kHashMatGrass:   slot = 2; break;
    case kHashMatWater:   slot = 3; break;
    case kHashMatWood:    slot = 4; break;
    case 0xB5CE36F5u:     slot = 5; break;
    case kHashMatMetal:   slot = 6; break;
    case kHashMatTile:    slot = 7; break;
    case kHashMatDirt:    slot = 8; break;
    case 0x1A811CBEu:     slot = 9; break;
    case kHashMatDefault: slot = 0xF; break;
    default:              slot = 0xF; break;
    }
    s32 idx = (slot == 0xF) ? 0 : slot;
    if (*(u8*)(arg0 + 0x57) != 0) {
        PlaySoundAt(*(void**)(arg0 + idx * 4 + 0x90), arg2, arg2 != NULL ? NULL : arg3, 0);
    }
    f32 pos[4];
    pos[0] = arg3[0];
    pos[1] = arg3[1];
    pos[2] = arg3[2];
    pos[3] = 0.0f;
    SpawnEffectAt(lbl_8047C760, *(u32*)(arg0 + idx * 4 + 0x68), pos, 0);
    if (arg2 != NULL) {
        if (((*(u32*)(arg0 + 0x60) >> 27) & 0xF) != 0) TriggerDoAction(arg0 + 0x60, arg1, arg2);
    } else if (((*(u32*)(arg0 + 0x64) >> 27) & 0xF) != 0) {
        TriggerDoAction(arg0 + 0x64, arg1, NULL);
    }
}

// 0x80138250
// Slot start: if slot is a valid index into the world's 0x30-byte slot table (size is the sum of
// the settings counts) and its state (+4) is 0, sets it to 5 and refreshes. Returns 1 if changed.
s32 fn_80138250(u8* arg0, s32 arg1) {
    s32 valid = 0;
    if (arg1 >= 0) {
        s32 sum = *(s32*)(GetGlobalSettings() + 0x4C);
        sum = *(s32*)(GetGlobalSettings() + 0x34) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x50) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x54) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x58) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x5C) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x60) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x64) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x38) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x3C) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x40) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x44) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x48) + sum;
        if (arg1 < sum) valid = 1;
    }
    u8* slot = valid ? *(u8**)(arg0 + 0xB0) + arg1 * 0x30 : NULL;
    if (slot != NULL && *(s32*)(slot + 0x4) == 0) {
        *(s32*)(slot + 0x4) = 5;
        fn_80132BC0(arg0);
        return 1;
    }
    return 0;
}

// 0x80138380
// Inverse of fn_80138250: if the slot's state (+4) is 5, resets it to 0 and refreshes. Returns 1 if changed.
s32 fn_80138380(u8* arg0, s32 arg1) {
    s32 valid = 0;
    if (arg1 >= 0) {
        s32 sum = *(s32*)(GetGlobalSettings() + 0x4C);
        sum = *(s32*)(GetGlobalSettings() + 0x34) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x50) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x54) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x58) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x5C) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x60) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x64) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x38) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x3C) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x40) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x44) + sum;
        sum = *(s32*)(GetGlobalSettings() + 0x48) + sum;
        if (arg1 < sum) valid = 1;
    }
    u8* slot = valid ? *(u8**)(arg0 + 0xB0) + arg1 * 0x30 : NULL;
    if (slot != NULL && *(s32*)(slot + 0x4) == 5) {
        *(s32*)(slot + 0x4) = 0;
        fn_80132BC0(arg0);
        return 1;
    }
    return 0;
}

// 0x8013A780
// Starts the multiplayer/online session when neither argument is set: reads each other
// player's 0x1000-byte block, and if the stored code is below our own, seeds the RNG and posts
// event 0x3D with a random 0-5 value; otherwise sets the alternate state.
void fn_8013A780(s32 arg0, s32 arg1) {
    OSLockMutex(lbl_80556AC8);
    s32 off = 0;
    lbl_805FB23C = 0;
    if (arg0 == 0 && arg1 == 0) {
        s32 i = 0;
        while (i < fn_802E9EE0()) {
            if (i != (s32)fn_802E9F3C()) {
                u8* buf = lbl_803F1068 + off;
                off += 0x1000;
                fn_802F79BC((u8)i, buf, 0x1000);
            }
            i++;
        }
        fn_801F5734((void*)1);
        bool start = false;
        if ((u32)lbl_805F9030 != 0x40) {
            u64 mine = fn_802FB094(lbl_80556E80);
            u8* ent = lbl_80556E80 + (lbl_805F9030 << 5);
            u64 stored = ((u64)*(u32*)(ent + 0x350) << 32) | *(u32*)(ent + 0x354);
            start = stored < mine;
        }
        if (start) {
            fn_801F51DC(1);
            srand((u32)fn_8033ADCC(NULL));
            if (fn_801F525C() == 0) fn_801F357C();
            u8* ev = (u8*)EventPost(0x3D, NULL);
            if (ev != NULL) {
                lbl_805FB23C = rand() % 6;
                *(s32*)(ev + 0x4) = lbl_805FB23C;
            }
            fn_80220A0C(0);
        } else {
            fn_801F51DC(0);
            fn_80220A0C(1);
        }
    }
    OSUnlockMutex(lbl_80556AC8);
}

// 0x8013ADD8
// Steps the list index (lbl_805FB35C) by dir (-1 / +1), clamped to 0..0x3C, and marks the four
// table entries at the new position as dirty; refreshes the UI when the index changed.
void fn_8013ADD8(s32 arg0) {
    u8* base = lbl_803F0D10;
    u32 old = (u32)lbl_805FB35C;
    if (arg0 == -1) {
        if (old != 0) {
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySound(kHashMenuHighlight, 0, 0, 0, 0, 0);
            s32 idx = lbl_805FB35C - 1;
            lbl_805FB35C = idx;
            for (s32 k = 0; k < 4; k++) {
                s32 e = *(s32*)(base + 0x120 + (idx + k) * 4);
                *(u32*)(base + 0x220 + e * 4) |= 1;
            }
            fn_8013AFF0();
        } else {
            lbl_805FB350 = 0;
        }
    } else if (arg0 == 1) {
        if (old < 0x3C) {
            if (lbl_805FBBAC == NULL) {
                void* v = fn_801D40E0(1);
                if (v != NULL) v = fn_80209814(v);
                lbl_805FBBAC = v;
            }
            PlaySound(kHashMenuHighlight, 0, 0, 0, 0, 0);
            s32 idx = lbl_805FB35C + 1;
            lbl_805FB35C = idx;
            for (s32 k = 0; k < 4; k++) {
                s32 e = *(s32*)(base + 0x120 + (idx + k) * 4);
                *(u32*)(base + 0x220 + e * 4) |= 1;
            }
            fn_8013AFF0();
        } else {
            lbl_805FB350 = 0;
        }
    }
    if ((u32)lbl_805FB35C != old) fn_80116EF0(base + 0x32C, lbl_805FB35C);
}

// 0x8013DE7C
// Leaves the creature preview: if no zone-loaded flag is clear, drops the preview parts back
// to the character, clears the flag list, re-enables the four hit-test groups.
void fn_8013DE7C(void) {
    if (lbl_805FB3DD != 0 && Creatures_GetCreatureLoaded() != 0) {
        s32* flags = (s32*)lbl_803C4E28;
        s32 n = lbl_805FB034;
        if (n > 0) {
            for (s32 i = 0; i < n; i++) {
                if (flags[i] == 0) return;
            }
        }
        for (s32 i = 0; i < 5; i++) {
            fn_8004DCEC(lbl_805FB3CC, i);
            u8* part = *(u8**)(lbl_805FB3CC + 0xC6C + i * 4);
            if (part != NULL) {
                void* p2 = fn_8004FFB4(part, *(void**)(lbl_803F20C4 + 4 + i * 4));
                if (p2 != NULL) {
                    fn_8004C718(lbl_805FB3CC, i, NULL);
                    fn_8004C718(lbl_805FB3CC, i, p2);
                    u8* obj = *(u8**)(lbl_805FB3CC + 0xC50 + i * 4);
                    if (obj != NULL) {
                        fn_8004EDD0(obj, 0, 0);
                        u8* obj2 = *(u8**)(lbl_805FB3CC + 0xC50 + i * 4);
                        *(u32*)(obj2 + 0x15C) |= 0x400000;
                    }
                }
                *(void**)(lbl_805FB3CC + 0xC6C + i * 4) = NULL;
            }
        }
        for (s32 i = 0; i < lbl_805FB034; i++) flags[i] = 0;
        lbl_805FB034 = 0;
        lbl_805FB038 = 0;
        if (lbl_805FB3C8 != NULL) {
            fn_801A3010(lbl_805FB3C8, lbl_805FB3CC);
            (*(VSlotFn*)(*(u8**)(lbl_805FB3C8 + 0x138) + 0x2C))(lbl_805FB3C8, 0x18);
            lbl_805FB3E4 = 5;
        }
        lbl_805FB3DD = 0;
        lbl_805FB3B5 = 1;
        UIGroup_EnableHitTest(0x4DD41D3Du);
        UIGroup_EnableHitTest(0x4D2C3770u);
        UIGroup_EnableHitTest(0x7CC42DEDu);
        UIGroup_EnableHitTest(0x38569AA8u);
    }
}

// 0x8013E0EC
// Enters the creature preview: disables the four hit-test groups, finds the preview element and
// character, clears old parts, and loads the preview creature setup.
void fn_8013E0EC(void) {
    if (lbl_805FB3DC == 1 && lbl_805FB3DD == 0) {
        UIGroup_DisableHitTest(0x4DD41D3Du);
        UIGroup_DisableHitTest(0x4D2C3770u);
        UIGroup_DisableHitTest(0x7CC42DEDu);
        UIGroup_DisableHitTest(0x38569AA8u);
        lbl_805FB3D4 = 0;
        void* set = ScreenSetsCurrent(gScreenSets);
        if (set != NULL) {
            lbl_805FB3C8 = (u8*)ScreenSetFindElement(set, 0x6530D136u);
            if (lbl_805FB3C8 != NULL) {
                fn_801A3010(lbl_805FB3C8, NULL);
                (*(VSlotFn*)(*(u8**)(lbl_805FB3C8 + 0x138) + 0x30))(lbl_805FB3C8, 0x1A);
            }
            if (lbl_805FB3CC != NULL) {
                for (s32 i = 0; i < 5; i++) {
                    u8* obj = *(u8**)(lbl_805FB3CC + 0xC50 + i * 4);
                    fn_8004C718(lbl_805FB3CC, i, NULL);
                    fn_8003DCE0(obj);
                }
            }
            lbl_805FB3CC = (u8*)CharacterFind(0x3E5D2A8E);
            if (lbl_805FB3CC != NULL) {
                fn_8020EDF0(lbl_803F20C4, lbl_803F2250);
                *(s32*)(lbl_803F20C4 + 0x44) = 0x39;
                fn_8004D760(lbl_805FB3CC, lbl_803F20C4);
                lbl_805FB3D0 = fn_8004DAF8(lbl_805FB3CC, 1, 1);
            }
        }
        lbl_805FB3DC = 0;
        lbl_805FB3EC = 0;
        lbl_805FB3DD = 1;
    }
}

// 0x8013F45C
// Updates a widget group: enables four child buttons (if the group exists), then shows/hides
// four icon children from bits 0/1/3/2 of arg1 and swaps the Alert_icon pair from bit 4.
void fn_8013F45C(u8* arg0, s32 arg1) {
    if (ScreenSetsCurrent(gScreenSets) == NULL) return;
    void* w;
    if (arg0 != NULL) {
        w = UIWidgetFindChild(arg0, 0x123437AAu);
        if (w != NULL) fn_8019CC7C(w, 1);
    }
    if (arg0 != NULL) {
        w = UIWidgetFindChild(arg0, 0x2C9F854Eu);
        if (w != NULL) fn_8019CC7C(w, 1);
    }
    if (arg0 != NULL) {
        w = UIWidgetFindChild(arg0, 0xA992CF63u);
        if (w != NULL) fn_8019CC7C(w, 1);
    }
    if (arg0 != NULL) {
        w = UIWidgetFindChild(arg0, 0x0980A68Du);
        if (w != NULL) fn_8019CC7C(w, 1);
    }
    w = UIWidgetFindChild(arg0, 0xDBC1241Fu);
    if (w != NULL) {
        s32 off = (arg1 & 1) ? 0x2C : 0x30;
        (*(VSlotFn*)(*(u8**)((u8*)w + 0x138) + off))(w, 2);
    }
    w = UIWidgetFindChild(arg0, 0x888C7061u);
    if (w != NULL) {
        s32 off = (arg1 & 2) ? 0x2C : 0x30;
        (*(VSlotFn*)(*(u8**)((u8*)w + 0x138) + off))(w, 2);
    }
    w = UIWidgetFindChild(arg0, 0x346927D7u);
    if (w != NULL) {
        s32 off = (arg1 & 8) ? 0x2C : 0x30;
        (*(VSlotFn*)(*(u8**)((u8*)w + 0x138) + off))(w, 2);
    }
    w = UIWidgetFindChild(arg0, 0xAF6D8A6Du);
    if (w != NULL) {
        s32 off = (arg1 & 4) ? 0x2C : 0x30;
        (*(VSlotFn*)(*(u8**)((u8*)w + 0x138) + off))(w, 2);
    }
    u8* a = (u8*)UIWidgetFindChild(arg0, kHashAlertIcon);
    u8* b = (u8*)UIWidgetFindChild(arg0, 0x6BEE3EBAu);
    if (a != NULL && b != NULL) {
        if (arg1 & 0x10) {
            (*(VSlotFn*)(*(u8**)(a + 0x138) + 0x30))(a, 2);
            (*(VSlotFn*)(*(u8**)(b + 0x138) + 0x2C))(b, 2);
        } else {
            (*(VSlotFn*)(*(u8**)(a + 0x138) + 0x2C))(a, 2);
            (*(VSlotFn*)(*(u8**)(b + 0x138) + 0x30))(b, 2);
        }
    }
}

// 0x8014210C
// Online-session poll: ticks the connection through the object's virtual status call (vtable
// +0x58) and, on an error state, shows the alert box that matches the error code.
void fn_8014210C(void) {
    if (lbl_805FB964 != NULL && fn_80142358() != 0 && fn_801F38F0() == 0) {
        if (fn_801F56FC(2) != 0) return;
        if (fn_801F57E0(lbl_805FB918) == 0) {
            fn_801F57B8();
            MatchupCleanup();
        }
        typedef u8 (*StatusFn)(void* self, s32* mode, s32* code);
        lbl_805FB444 = (*(StatusFn*)(*(u8**)lbl_805FB964 + 0x58))(lbl_805FB964, &lbl_805FB440, &lbl_805FB448);
        if (lbl_805FB444 != 0 && lbl_805FB440 < 7 && lbl_805FB440 >= 3) {
            MultiplayerCleanup();
        }
        if (lbl_805FB444 == 1 && lbl_805FB44C == 0) {
            lbl_805FB44C = 1;
            s32 c = __cvt_fp2signed(lbl_805FB448);
            if ((c >= 0x4E86 && c <= 0x4E8D) || (c >= 0x4E8F && c <= 0x5207)) {
                ShowAlertBox(0x8554F56Cu);
            } else if (c == 0x4E85 || (c >= 0x59D8 && c <= 0x5DBF)) {
                ShowAlertBox(0x0FDD8F3Au);
            } else if (c == 0x4E8E) {
                ShowAlertBox(0x8F2E6505u);
            } else if (c == 0x7148) {
                ShowAlertBox(0x0D84AA48u);
            } else if (c == 0x7149) {
                ShowAlertBox(0x340550DDu);
            } else if (c == 0x4E84 || (c >= 0xC350 && c <= 0xEA5F)) {
                ShowAlertBox(0xEB4BCABFu);
            } else if ((u32)(c - 0x10000) == 0x3A2Eu) {
                ShowAlertBox(0xD48D7CA1u);
            } else if (lbl_805FB440 == 7) {
                ShowAlertBox(0xBD65120Au);
            } else if ((u32)(lbl_805FB440 - 3) <= 3u) {
                ShowAlertBox(0xC0189F1Eu);
            } else if (c < 0x7918 || c > 0x7CFF) {
                ShowAlertBox(0x5A9872D0u);
            } else {
                ShowAlertBox(0x5639D127u);
            }
        }
        if (lbl_805FB454 == 1) {
            lbl_805FB454 = 0;
            UIScreen_PopScreen(0x28DCC9E9u);
        }
    }
}

// 0x80143158
// Initializes a log/file record: clears state fields, default path "./", and the month and
// weekday abbreviation tables. Returns the record.
u8* fn_80143158(u8* arg0) {
    *(s32*)(arg0 + 0x304) = 0;
    *(u8*)(arg0 + 0x4) = 0;
    *(u8*)(arg0 + 0x104) = 0;
    *(s32*)(arg0 + 0x0) = 0;
    *(s32*)(arg0 + 0x308) = 0;
    strcpy((char*)(arg0 + 0x458), "./");
    strcpy((char*)(arg0 + 0x30C), "Jan");
    strcpy((char*)(arg0 + 0x310), "Feb");
    strcpy((char*)(arg0 + 0x314), "Mar");
    strcpy((char*)(arg0 + 0x318), "Apr");
    strcpy((char*)(arg0 + 0x31C), "May");
    strcpy((char*)(arg0 + 0x320), "Jun");
    strcpy((char*)(arg0 + 0x324), "Jul");
    strcpy((char*)(arg0 + 0x328), "Aug");
    strcpy((char*)(arg0 + 0x32C), "Sep");
    strcpy((char*)(arg0 + 0x330), "Oct");
    strcpy((char*)(arg0 + 0x334), "Nov");
    strcpy((char*)(arg0 + 0x338), "Dec");
    strcpy((char*)(arg0 + 0x33C), "Sun");
    strcpy((char*)(arg0 + 0x340), "Mon");
    strcpy((char*)(arg0 + 0x344), "Tue");
    strcpy((char*)(arg0 + 0x348), "Wed");
    strcpy((char*)(arg0 + 0x34C), "Thu");
    strcpy((char*)(arg0 + 0x350), "Fri");
    strcpy((char*)(arg0 + 0x354), "Sat");
    return arg0;
}

// 0x80143350
// (Re)opens the log file: closes any open file, builds the path from the directory (+0x458) and
// the two names (+4, +0x104), opens it ("wt" when flag bit 0 is set, else "at"). Returns 1 on a
// usable record, 0 if either name is empty.
s32 fn_80143350(u8* arg0) {
    if (*(void**)(arg0 + 0x0) != NULL) {
        fclose(*(void**)(arg0 + 0x0));
        *(void**)(arg0 + 0x0) = NULL;
    }
    s8 nameA = *(s8*)(arg0 + 0x4);
    s32 ok = (nameA != 0 && *(s8*)(arg0 + 0x104) != 0) ? 1 : 0;
    if (ok == 0) return 0;
    const char* a = (nameA != 0) ? (const char*)(arg0 + 0x4) : (const char*)(lbl_80375E18 + 0xC);
    const char* b = (*(s8*)(arg0 + 0x104) != 0) ? (const char*)(arg0 + 0x104) : (const char*)(lbl_80375E18 + 0x34);
    SysSprintf((char*)(arg0 + 0x204), (const char*)(lbl_80375E18 + 0x1E0), (char*)(arg0 + 0x458), a, b);
    if (*(u32*)(arg0 + 0x308) & 1) {
        *(void**)(arg0 + 0x0) = UTF8ToUCS2String((const char*)(arg0 + 0x204), "wt");
    } else {
        *(void**)(arg0 + 0x0) = UTF8ToUCS2String((const char*)(arg0 + 0x204), "at");
    }
    if (*(void**)(arg0 + 0x0) != NULL) *(s32*)(arg0 + 0x304) = 2;
    return 1;
}

// 0x8014474C
// Converts the rotation matrix at arg0 (columns at +0/+0x10/+0x20) to a unit quaternion at
// arg1 (x,y,z,w), picking the largest diagonal term, then normalizes with two rsqrt steps.
void fn_8014474C(u8* arg0, f32* arg1) {
    f32 a = *(f32*)(arg0 + 0x0);
    f32 b = *(f32*)(arg0 + 0x10);
    f32 c = *(f32*)(arg0 + 0x20);
    f32 tr = c + (a + b);
    f32 x, y, z, w;
    if (a >= b && a >= c && a >= tr) {
        f32 t = (a - b) - c;
        y = *(f32*)(arg0 + 0xC) + *(f32*)(arg0 + 0x4);
        x = 1.0f + t;
        z = *(f32*)(arg0 + 0x18) + *(f32*)(arg0 + 0x8);
        w = *(f32*)(arg0 + 0x1C) - *(f32*)(arg0 + 0x14);
    } else if (b >= c && b >= tr) {
        f32 t = (b - a) - c;
        x = *(f32*)(arg0 + 0xC) + *(f32*)(arg0 + 0x4);
        y = 1.0f + t;
        z = *(f32*)(arg0 + 0x1C) + *(f32*)(arg0 + 0x14);
        w = *(f32*)(arg0 + 0x8) - *(f32*)(arg0 + 0x18);
    } else if (c >= tr) {
        f32 t = (c - a) - b;
        x = *(f32*)(arg0 + 0x18) + *(f32*)(arg0 + 0x8);
        y = *(f32*)(arg0 + 0x1C) + *(f32*)(arg0 + 0x14);
        z = 1.0f + t;
        w = *(f32*)(arg0 + 0xC) - *(f32*)(arg0 + 0x4);
    } else {
        f32 t = c + (a + b);
        x = *(f32*)(arg0 + 0x1C) - *(f32*)(arg0 + 0x14);
        y = *(f32*)(arg0 + 0x8) - *(f32*)(arg0 + 0x18);
        z = *(f32*)(arg0 + 0xC) - *(f32*)(arg0 + 0x4);
        w = 1.0f + t;
    }
    if (w > 0.0f) {
        arg1[0] = -x;
        arg1[1] = -y;
        arg1[2] = -z;
        arg1[3] = w;
    } else {
        arg1[0] = x;
        arg1[1] = y;
        arg1[2] = z;
        arg1[3] = -w;
    }
    f32 q0 = arg1[0];
    f32 q1 = arg1[1];
    f32 q2 = arg1[2];
    f32 q3 = arg1[3];
    f32 sq = q3 * q3 + (q2 * q2 + (q0 * q0 + q1 * q1));
    f32 inv = 1.0f / sqrtf(sq);  // original: frsqrte with two Newton steps
    arg1[0] = q0 * inv;
    arg1[1] = q1 * inv;
    arg1[2] = q2 * inv;
    arg1[3] = q3 * inv;
}

// 0x80146FD8
// Base destructor: unregisters the object from the global table at lbl_805FB488 (count at +0xA0,
// pointers from +0x20, compacting the list) when its id (u16 at +0) is nonzero, then frees if
// del > 0. Returns the object.
u8* fn_80146FD8(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        if (*(s32*)(arg0 + 0x4) != 0) {
            u16 id = *(u16*)(arg0 + 0x0);
            *(s32*)(arg0 + 0x4) = 0;
            *(s32*)(arg0 + 0x8) = 1;
            s32 found = (id != 0);
            if (found) {
                s32 f = 0;
                u8* t = lbl_805FB488;
                if (t != NULL) {
                    u32 n = *(u32*)(t + 0xA0);
                    for (u32 i = 0; i < n; i++) {
                        if (*(u8**)(t + 0x20 + i * 4) == arg0) {
                            f = 1;
                            break;
                        }
                    }
                }
                found = (f != 0);
            }
            if (found && lbl_805FB488 != NULL) {
                u8* t = lbl_805FB488;
                u32 n = *(u32*)(t + 0xA0);
                for (u32 i = 0; i < n; i++) {
                    if (*(u8**)(t + 0x20 + i * 4) == arg0) {
                        u8* slot = t + i * 4;
                        *(void**)(slot + 0x20) = NULL;
                        *(u32*)(t + 0xA0) = *(u32*)(t + 0xA0) - 1;
                        while (i < *(u32*)(t + 0xA0)) {
                            *(void**)(slot + 0x20) = *(void**)(slot + 0x24);
                            i++;
                            slot += 4;
                        }
                        *(void**)(t + *(u32*)(t + 0xA0) * 4 + 0x20) = NULL;
                        break;
                    }
                }
            }
        }
        if (arg1 > 0) operator_delete(arg0);
    }
    return arg0;
}

}  // extern "C"
