// Decompiled functions: UI script callbacks, spawn helpers, lightning/laser, camera/NIS misc.
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "game/node_ref.h"
#include "game/part.h"
#include "game/character.h"
#include "engine/engine.h"
extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C" {
void*       ScreenSetsCurrent(void* sets);                 // 0x8018D1E4
void*       ScreenSetFindElement(void* set, u32 hash);     // 0x80198F18
void        fn_8018D2B8(const char* fmt, ...);             // 0x8018D2B8 (debug log)
void        fn_8018D370(const char* fmt, ...);             // 0x8018D370 (debug log)
const char* FormatHashString(u32 hash);                         // 0x801EB708 (hash -> name for logs)
const char* UITextGetString(void* text);                   // 0x8019F3AC
u32         fn_801A65A0(void* obj);                        // 0x801A65A0
u32         fn_801A665C(void* obj);                        // 0x801A665C
u8*         fn_801A7250(u8* grid);                         // 0x801A7250
u8*         UIGridItem_FindGroupType(u8* a, s32 idx);                   // 0x801A9EFC
s32 fn_801A6B9C(u8* grid, s32 a);                  // 0x801A6B9C


void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void* EventPost(s32 id, void* sender);                      // 0x801F3918 (returns payload)
s32   WorldGetPhase(void* world);                           // 0x800D83A8
BOOL  ZoneIsLoaded(void* world, s32 zone);                  // 0x8012F408
void  fn_8018DEF4(void* sets);                              // 0x8018DEF4
void fn_800D816C(u8* arg0, s32 arg1);                        // 0x800D816C
void  ConversationHideHUDInfo(void);                        // 0x8004xxxx (draft_miss9.cpp)
void  SessionResetUnk204(u8* s);                            // 0x80013684
void  fn_8020A474(s32 handle);                              // 0x8020A474
void  UnPauseAllExplorationEnemies(void);                   // 0x80016DB0
void  GameFlowSignal(s32 id, s32 a, s32 b);                 // 0x8000C7F4
BOOL  PathIsAtJunction(PathFollower* p);                    // 0x800D5924
void  fn_800D5C48(u8* arg0, s32 arg1);
void  fn_80016FD0(Character* c, int animate);
void* ScreenSetFindWidget(void* set, u32 hash);             // 0x80198E34
void  ScreenSetsFlush(void* sets);
u8*   fn_800C79C0(u8* a);                                    // 0x800C79C0
u8*   fn_800C73EC(u8* h);                                    // 0x800C73EC
s32   fn_801B5510(u8* a, u8* b, s32 c, f32* v, s32 d);      // 0x801B5510
void  fn_802188A8(u8* lab, u8* entry);                      // 0x802188A8 (stores r4 at lab+0x48)
u8*   ObjDbLookup(void* db, s32 id);
u8*   ObjRefDtor(u8* ref, s32 flags);
Vec3* PathGetPosition(PathFollower* p);                     // 0x800D5xxx
void  fn_80053418(u8* arg0);                                // 0x80053418
void* fn_8004D7E0(u8*, s32, s32);                           // 0x8004D7E0
s32   PartGetCurrentHealth(const PartInstance* p);          // 0x8021EF68
void  fn_800D6B0C(s32* rec);                                // 0x800D6B0C (resets a spawn record)
void  fn_800D70EC(s32* rec, f32 amount);                    // 0x800D70EC (registers a spawn record)
s32   fn_800D4730(void* db, s32 id);                        // 0x800D4730

extern u8 gGame[];                         // 0x803B53A0
extern u8 gWorldState[];                   // 0x803E6D00
extern u8 lbl_803C4298[];                  // 0x803C4298 (+4 camera object, +8 camera target data)
extern u8 lbl_805FB0E1;
extern s32* lbl_805FB0F4;
extern u32 lbl_805FB0F0;
extern void* lbl_805FB0DC;
extern u8 lbl_805FB0E0;
extern s32 lbl_805FB0E8;
extern s32 lbl_805FB0F8;
extern f32 lbl_805FE510;                   // 0.0f
extern f32 lbl_805FEB0C;                   // 0.0f
extern f32 lbl_805FEB80;                   // 0.8f
extern f32 lbl_805FEB48;                   // 2.0f
extern const char lbl_80381868[];          // 0x80381868 (string block)
extern f32 lbl_805FF8A0;                   // 0x805FF8A0 (0.0f)
extern f32 lbl_805FF8A4;                   // 0x805FF8A4 (1.0f)
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void* EventPost(s32 id, void* sender);                      // 0x801F3918 (returns payload)
s32   WorldGetPhase(void* world);                           // 0x800D83A8
BOOL  ZoneIsLoaded(void* world, s32 zone);                  // 0x8012F408
void  fn_8018DEF4(void* sets);                              // 0x8018DEF4
void fn_800D816C(u8* arg0, s32 arg1);                        // 0x800D816C
void  SessionResetUnk204(u8* s);                            // 0x80013684
void  fn_8020A474(s32 handle);                              // 0x8020A474
BOOL  PathIsAtJunction(PathFollower* p);                    // 0x800D5924
void  fn_800D5C48(u8* arg0, s32 arg1);
void  fn_80016FD0(Character* c, int animate);
void* ScreenSetFindWidget(void* set, u32 hash);             // 0x80198E34
void  ScreenSetsFlush(void* sets);
u8*   fn_800C79C0(u8* a);                                    // 0x800C79C0
u8*   fn_800C73EC(u8* h);                                    // 0x800C73EC
s32   fn_801B5510(u8* a, u8* b, s32 c, f32* v, s32 d);      // 0x801B5510
void  fn_802188A8(u8* lab, u8* entry);                      // 0x802188A8 (stores r4 at lab+0x48)
u8*   ObjDbLookup(void* db, s32 id);
u8*   ObjRefDtor(u8* ref, s32 flags);
Vec3* PathGetPosition(PathFollower* p);
void  fn_80053418(u8* arg0);                                // 0x80053418
void* fn_8004D7E0(u8*, s32, s32);                           // 0x8004D7E0
s32   PartGetCurrentHealth(const PartInstance* p);          // 0x8021EF68
void  fn_800D6B0C(s32* rec);                                // 0x800D6B0C (resets a spawn record)
void  fn_800D70EC(s32* rec, f32 amount);                    // 0x800D70EC (registers a spawn record)
s32   fn_800D4730(void* db, s32 id);                        // 0x800D4730
u8*   fn_801D40E0(s32 n);                                   // 0x801D40E0 (alloc)
u8*   SaveGetCurrentMonster(u8* s);
s32   SysSprintf(char* buf, const char* fmt, ...);          // 0x80142EFC
void  fn_8001F6A4(u8* t);                                   // 0x8001F6A4
s32   fn_801138F8(void);                                    // 0x801138F8
s32   fn_8013E454(u8* mon, u8* save);                       // 0x8013E454
u8    PartGetTier(const PartInstance* p);
s8    PartGetQuality(const PartInstance* p);

extern u8 gGame[];                         // 0x803B53A0
extern u8 gWorldState[];                   // 0x803E6D00
extern u8 lbl_803C4298[];                  // 0x803C4298 (+4 camera object, +8 camera target data)
extern u8 lbl_805FB0E1;
extern s32* lbl_805FB0F4;
extern u32 lbl_805FB0F0;
extern void* lbl_805FB0DC;
extern u8 lbl_805FB0E0;
extern s32 lbl_805FB0E8;
extern s32 lbl_805FB0F8;
extern f32 lbl_805FE510;                   // 0.0f
extern f32 lbl_805FEB0C;                   // 0.0f
extern f32 lbl_805FEB80;                   // 0.8f
extern f32 lbl_805FEB48;                   // 2.0f
extern u8* lbl_805FAF80;
extern s32 lbl_805FAF84;
extern u8* lbl_805FBBE0;                   // save block pointer
extern s32 lbl_805F90A0;
extern char lbl_803F2210[];
extern const char lbl_805F828C[];          // "%d/%d"
extern f32 lbl_805FB3F0;
extern f32 lbl_805FB3F4;
extern f32 lbl_805FB3F8;
extern f32 lbl_805FB3FC;
void  ScopedLockCtor(ScopedLock* l);
void  ScopedLockDtor(ScopedLock* l, s32 flags);
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);   // 0x801C80E0
extern u8 lbl_8047C760[];
extern f32 lbl_805FD598;                   // 0.0f
extern f32 lbl_805FD5A0;                   // 0.0001f
extern f32 lbl_805FD5A4;                   // -0.0001f
extern f32 lbl_805FD5A8;                   // 1.0f
void  fn_801B55F4(u8* obj, u8* out, s32 idx);               // 0x801B55F4
void  fn_800E9E54(f32* out, u8* ent, u8* loc, u32 hash);    // 0x800E9E54
s32   fn_80065200(u8* arg0, u8* arg1, void* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6);   // 0x80065200
void* SpawnHelper(void* owner, s32 arg);                    // 0x8000C478
u32   RandomU32(void);                                      // 0x80143460
extern f32 lbl_805FEB04;                   // 1.0f
extern f32 lbl_805FEB1C;                   // 2^-32
extern f32 lbl_805FEB38;                   // 0.7853982f (pi/4)
s32   fn_801ED368(u8* ref);                                 // 0x801ED368
u32   fn_801ED2FC(u8* ref);                                 // 0x801ED2FC
s32   fn_801ED538(u8* ref);                                 // 0x801ED538
void* fn_801B7198(void* anim, s32 a);                       // 0x801B7198
void  fn_801ED788(f32* out, u8* a1);                        // 0x801ED788
s32   fn_8012EE30(u8* world, f32* pos);                     // 0x8012EE30
void  fn_801B1778(void* p, s32 a);                          // 0x801B1778
void  fn_801E8D74(void* out, void* list, void* owner, void* anim);   // 0x801E8D74
u8* fn_801C3EA4(u8* table, s32 key);                            // 0x801C3EA4
s32   fn_801C5C60(s32 a);                                   // 0x801C5C60
s32   fn_800D59D4(u8* a3);                                  // 0x800D59D4
void* fn_8004D968(u8*, s32, s32);                           // 0x8004D968
s32  fn_800ED944(u8* p, void* owner, f32 dt);                       // 0x800ED944 (lightning callback)
u32   StringHash(const char* s);
extern u8 lbl_8047C7AC[];
extern s32 lbl_805FB1D8[];
extern s32 lbl_805FB1E0[];
extern s32 lbl_805FB6B8;
extern u32 lbl_805FB1EC;
extern u8 lbl_805FB1F0;
extern const char lbl_8036045C[];
s32 fn_801F1EB0(u8* arg0, f32 dt);                                 // 0x801F1EB0
s32   fn_800FB444(void);                                    // 0x800FB444
s32   fn_800FB45C(void);                                    // 0x800FB45C
void fn_80206F14(u8* p, const char* title, const char* subtitle);   // 0x80206F14
u32   fn_80143974(s32 seed, void* data, u32 len);           // 0x80143974 (checksum)
void* MemAllocSmall(u32 size);                              // 0x801D41CC
void  MemFreeSmall(void* p);                                // 0x801D4340
char* fn_80338BC0(char* s, const char* sub);                // 0x80338BC0 (MSL strstr)
void  RendBeginFrame(void);                                 // 0x8014F0A0
void  RendEndFrame(void);                                   // 0x801508AC
void  RendClear(int flags, int a, int b, f32 depth);        // 0x801534F0
void  fn_80150148(void);
void  fn_801FC0A0(void);
void  fn_801FC1EC(void);
void  fn_80188ED4(s32 a);                                   // 0x80188ED4
void  fn_80188E5C(u8* p);                                   // 0x80188E5C
void  fn_80188EDC(const char* text, s32* w, s32* h);        // 0x80188EDC (text size)
void  fn_80188F00(const char* fmt, ...);                    // 0x80188F00
extern u8 lbl_80556E80[];
extern u8 lbl_80348F38[];                  // dispatch table
extern u8 lbl_80412BE0[];                  // screen size record
extern const char lbl_805F8200[];
extern const char lbl_805F8FD4[];
extern const char lbl_805F96C0[];          // default system message
extern const char lbl_805F96C8[];          // line separator
extern const char lbl_805F96CC[];          // line format
extern f32 lbl_80600078;
extern s32 lbl_805FB51C;
extern s32 lbl_805FB63C;
extern s32 lbl_805FB630;
extern s32 lbl_805FB628;
extern s32 lbl_805FB62C;
extern s32 lbl_805FBA30;
// @@DECLS@@
}
extern "C++" {
extern "C" void fn_80142970(void);                                    // 0x80142970 (runs registered per-frame callbacks)
u8*   fn_801B55A8(u8* ent);                                  // 0x801B55A8 (locator count, returned as an integer value)
f32   ComputeLobSpeed(const Vec3* from, const Vec3* to, f32 angle, f32 gravity);   // 0x80019FB0
}
struct StringTable;
extern "C++" StringTable* gStringTable;      // 0x805FB910
extern "C++" void* gScreenSets;            // 0x805FB654
extern "C++" void* gEventMgr;              // 0x805FB6C8
extern "C++" void* gObjDb;
extern "C++" s32   gZoneResourceSize;      // 0x805FB94C
extern "C++" void* gLab;                   // 0x805FBBC8

typedef void (*ElemVecFn)(void* el, s32 code, const f32* v);
typedef void (*ElemFloatFn)(void* el, f32 v);

// Element lookup in the current screen set, logging with the strings at lbl_80381868.
static u8* FindElement48(u32 hash) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) {
        fn_8018D370(lbl_80381868 + 0x40);
        return NULL;
    }
    u8* el = (u8*)ScreenSetFindElement(set, hash);
    if (el == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x238, FormatHashString(hash));
        return NULL;
    }
    return el;
}

// Returns the grid element for hash, or NULL (logs) if it is not a grid (type 4).
static u8* FindGrid48(u32 hash) {
    u8* el = FindElement48(hash);
    if (el == NULL) return NULL;
    if (*(s32*)(el + 0x60) != 4) {
        fn_8018D370(lbl_80381868 + 0x558, FormatHashString(hash));
        return NULL;
    }
    return el;
}

// Resolves the child locator selected by spec (u16 flags at +0: bit 0 = bone index in byte +2,
// bit 1 = locator-table index in byte +2) into out (0x30 bytes; position at +0x24). False if
// the index is out of range / unset.
static BOOL GetChildLocator48(u8* ent, u8* spec, u8* out) {
    u16 flags = *(u16*)spec;
    s32 idx = (flags & 1) ? (s32)*(u8*)(spec + 2) : -1;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(ent)) {
            fn_801B55F4(ent, out, idx);
            return 1;
        }
        return 0;
    }
    idx = (flags & 2) ? (s32)*(u8*)(spec + 2) : -1;
    if (idx != -1) {
        u8* r = *(u8**)(ent + 0x98) + idx * 0xC8;
        for (s32 i = 0; i < 12; i++) *(f32*)(out + i * 4) = *(f32*)(r + 0x38 + i * 4);
        return 1;
    }
    return 0;
}

// Hash of the lightning effect name, computed once and cached in lbl_805FB1EC.
static u32 LightningHash48(void) {
    if ((s8)lbl_805FB1F0 == 0) {
        lbl_805FB1EC = StringHash(lbl_8036045C);
        lbl_805FB1F0 = 1;
    }
    return lbl_805FB1EC;
}

// @@HELPERS@@
// Hides the conversation widget (hash 0xE7667FBE) in the HUD screen set via its vtable slot 0x2C.
static void ConversationHideWidget48(void) {
    u8* set = *(u8**)((u8*)gScreenSets + 0x88);
    if (set != NULL) {
        u8* w = (u8*)ScreenSetFindWidget(set, 0xE7667FBEu);
        if (w != NULL) {
            (*(void (**)(void*, s32))(*(u8**)(w + 0x138) + 0x2c))(w, 2);
        }
    }
}

extern "C" {

// 0x8018F92C
// Sets the element position (+0x68..) and notifies it through vtable slot 0x2C (code 0x18).
void UIElement_SetPos_Floats(u32 a0, f32 a1, f32 a2, f32 a3) {
    u8* el = FindElement48(a0);
    if (el != NULL) {
        f32 v[3] = {a1, a2, a3};
        *(f32*)(el + 0x68) = v[0];
        *(f32*)(el + 0x6C) = v[1];
        *(f32*)(el + 0x70) = v[2];
        (*(ElemVecFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 0x18, v);
    }
}

// 0x8018FD54
// Sets the element size (+0x74..) and notifies it through vtable slot 0x2C (code 0x18).
void UIElement_SetSize_Floats(u32 a0, f32 a1, f32 a2, f32 a3) {
    u8* el = FindElement48(a0);
    if (el != NULL) {
        f32 v[3] = {a1, a2, a3};
        *(f32*)(el + 0x74) = v[0];
        *(f32*)(el + 0x78) = v[1];
        *(f32*)(el + 0x7C) = v[2];
        (*(ElemVecFn*)(*(u8**)(el + 0x138) + 0x2c))(el, 0x18, v);
    }
}

// 0x80193398
// Moves the grid cursor to a flat index (row = idx / cols, col = idx % cols), keeping the
// highlight centred / clamped depending on the scroll flags, and records the previous cell.
void UIGrid_SetCurrentIndex(u32 a0, s32 a1) {
    u8* g = FindGrid48(a0);
    if (g == NULL) return;

    s32 cols = *(s32*)(g + 0x140);
    s32 row = a1 / cols;
    s32 col = a1 % cols;
    s32 c = *(s32*)(g + 0x148) / 2;
    s32 colCur;                 // new value for +0x150
    s32 limC = (s32)fn_801A65A0(g);
    s32 flags = *(s32*)(g + 0x64);
    if (flags & 0x40000) {
        c = *(s32*)(g + 0x148) / 2;
        colCur = c;
    } else if (flags & 0x10000) {
        colCur = c;
    } else {
        s32 n = *(s32*)(g + 0x148);
        if (n < *(s32*)(g + 0x140)) {
            if (col - c < 0) {
                colCur = col;
                c = col;
            } else if (col + c > limC) {
                if (limC <= n) {
                    colCur = col;
                    c = col;
                } else {
                    c = n - 1;
                    colCur = c;
                }
            } else {
                colCur = c;
            }
        } else {
            c = 0;
            colCur = col;
        }
    }

    s32 r = *(s32*)(g + 0x144) / 2;
    s32 rowCur;                 // new value for +0x14C
    s32 limR = (s32)fn_801A665C(g);
    flags = *(s32*)(g + 0x64);
    if (flags & 0x200000) {
        r = *(s32*)(g + 0x144) / 2;
        rowCur = r;
    } else if (flags & 0x80000) {
        rowCur = r;
    } else {
        s32 n = *(s32*)(g + 0x144);
        if (n < *(s32*)(g + 0x13C)) {
            if (row - r < 0) {
                rowCur = row;
                r = row;
            } else if (row + r > limR) {
                if (limR <= n) {
                    rowCur = row;
                    r = row;
                } else {
                    r = n - 1;
                    rowCur = r;
                }
            } else {
                rowCur = r;
            }
        } else {
            r = 0;
            rowCur = row;
        }
    }

    if ((flags & 0x50000) || *(s32*)(g + 0x148) < *(s32*)(g + 0x140)) {
        s32 v = col - c;
        *(s32*)(g + 0x160) = v;
        if (v < 0) *(s32*)(g + 0x160) = v + *(s32*)(g + 0x140);
    }
    if ((*(s32*)(g + 0x64) & 0x280000) || *(s32*)(g + 0x144) < *(s32*)(g + 0x13C)) {
        s32 v = row - r;
        *(s32*)(g + 0x15C) = v;
        if (v < 0) *(s32*)(g + 0x15C) = v + *(s32*)(g + 0x13C);
    }
    *(s32*)(g + 0x154) = *(s32*)(g + 0x14C);
    *(s32*)(g + 0x14C) = rowCur;
    *(s32*)(g + 0x158) = *(s32*)(g + 0x150);
    *(s32*)(g + 0x150) = colCur;
    *(u8*)(g + 0x17C) = 0;
    if (*(s32*)(g + 0x64) & 0x800000) *(f32*)(g + 0x174) = lbl_805FF8A4;
    fn_801A6B9C(g, 1);
    (*(ElemFloatFn*)(*(u8**)(g + 0x138) + 0x18))(g, lbl_805FF8A0);
}

// 0x80193688
// Current grid cell as a flat index (row * cols + col), with the scroll offsets applied.
s32 UIGrid_GetCurrentIndex_ToScript(u32 a0) {
    u8* g = FindGrid48(a0);
    if (g == NULL) return 0;
    s32 rows = *(s32*)(g + 0x13C);
    s32 row = *(s32*)(g + 0x14C) + *(s32*)(g + 0x15C);
    s32 col = *(s32*)(g + 0x150) + *(s32*)(g + 0x160);
    if (row >= rows) row -= rows;
    s32 cols = *(s32*)(g + 0x140);
    if (col >= cols) col -= cols;
    return col + row * cols;
}

// 0x8019378C
// Previous grid cell as a flat index; -1 when the grid has no previous cell (byte +0x16C == 0).
s32 UIGrid_GetPreviousIndex_ToScript(u32 a0) {
    u8* g = FindGrid48(a0);
    if (g == NULL) return 0;
    if (*(u8*)(g + 0x16C) == 0) return -1;
    s32 rows = *(s32*)(g + 0x13C);
    s32 row = *(s32*)(g + 0x154) + *(s32*)(g + 0x15C);
    s32 col = *(s32*)(g + 0x158) + *(s32*)(g + 0x160);
    if (row >= rows) row -= rows;
    s32 cols = *(s32*)(g + 0x140);
    if (col >= cols) col -= cols;
    return col + row * cols;
}

// 0x801938A4
// Text of the grid current cell, or 0 when empty / unavailable. (Registered with an int
// return type; the value is really a string pointer.)
s32 UIGrid_GetCurrentText(u32 a0) {
    u8* g = FindGrid48(a0);
    if (g == NULL) return 0;
    u8* cur = fn_801A7250(g);
    if (cur == NULL) {
        fn_8018D370(lbl_80381868 + 0x580);
        return 0;
    }
    u8* cell = UIGridItem_FindGroupType(cur, 1);
    if (cell == NULL) {
        fn_8018D370(lbl_80381868 + 0x5A8);
        return 0;
    }
    if (*UITextGetString(cell) == 0) return 0;
    return (s32)(intptr_t)UITextGetString(cell);
}

// 0x80194628
// Sets the grid highlight colour (RGBA floats at +0x13C..+0x148 of the highlight object).
void UIGrid_SetHighlightColor(u32 a0, f32 a1, f32 a2, f32 a3, f32 a4) {
    u8* g = FindGrid48(a0);
    if (g == NULL) return;
    u8* h = *(u8**)(g + 0x1B4);     // fn_801A7DB8(g): leaf returning the word at +0x1B4
    if (h == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x5DC);
        return;
    }
    f32 v[8] = {a1, a2, a3, a4, a1, a2, a3, a4};
    *(f32*)(h + 0x13C) = a1;
    *(f32*)(h + 0x140) = a2;
    *(f32*)(h + 0x144) = a3;
    *(f32*)(h + 0x148) = a4;
    (*(ElemVecFn*)(*(u8**)(h + 0x138) + 0x2c))(h, 0x20, v);
}

// 0x800F695C
// Starts a cutscene (NIS) state machine: flushes the screen sets, optionally switches the world
// to mode a0 and hides the conversation HUD, then (in exploration phase 2) wakes every entity
// that supports it with the 0x4F9213C1 event.
void StartNISFSM(u32 a0) {
    if (*(s32*)(gGame + 0xFE0) == 0 || WorldGetPhase(gGame + 0xD5F0) == 6) {
        fn_8018DEF4(gScreenSets);
        if (a0 != 0) {
            fn_800D816C(gGame + 0xD5F0, (s32)a0);
            lbl_805FB0E1 = 1;
            ConversationHideHUDInfo();
        }
        if (WorldGetPhase(gGame + 0xD5F0) == 2) {
            for (s32 i = 0; i < 0x20; i++) {
                u8* e = *(u8**)(gGame + 0x140 + i * 4);
                if (e != NULL && (*(s32 (**)(void*))(*(u8**)e + 0x4C))(e) == 0) {
                    e = *(u8**)(gGame + 0x140 + i * 4);
                    if ((*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x10) && *(u8*)(e + 0x895) == 0) {
                        EventSend(gEventMgr, e, NULL, 0x4F9213C1u, 0, 0, 0, 0);
                    }
                }
            }
        }
    }
}

// 0x800CC52C
// Stitching reticle: once the shot is ready, looks up the weapon slot's transformed origin and
// tries to fire; on success clears bit 0 of the flags in the object returned by fn_800C73EC.
void Stitching_Reticle_ReadyToFire(void* a0) {
    u8* r = (u8*)a0;
    if (r != NULL && *(s32*)(r + 0x268) == 0 && *(s32*)(r + 0x26C) != 0) {
        u8* v = fn_800C79C0((u8*)*(u8**)(r + 0x264));
        *(u8**)(r + 0x268) = v;
        if (v != 0) {
            u8* obj = fn_800C73EC(v);
            if (obj != NULL) {
                const f32 k = lbl_805FE510;
                u8* m = *(u8**)(r + 0x98) + *(s32*)(r + 0x270) * 0xC8;
                // transform of the (k, k, k) point by the 3x4 matrix at m + 8
                f32 pos[3];
                pos[0] = *(f32*)(m + 0x2C) + (*(f32*)(m + 0x20) * k + (*(f32*)(m + 0x8) * k + *(f32*)(m + 0x14) * k));
                pos[1] = *(f32*)(m + 0x30) + (*(f32*)(m + 0x24) * k + (*(f32*)(m + 0xC) * k + *(f32*)(m + 0x18) * k));
                pos[2] = *(f32*)(m + 0x34) + (*(f32*)(m + 0x28) * k + (*(f32*)(m + 0x10) * k + *(f32*)(m + 0x1C) * k));
                if (fn_801B5510(r, obj, *(s32*)(m + 0x4), pos, 1) != 0) {
                    *(s32*)(*(u8**)(obj + 0x4) + 0x20) &= ~1;
                }
            }
        }
    }
}

// 0x800512FC
// Ends the current conversation: releases its handle and state, clears button presses, tells the
// player entity (event 0x6877205C), resumes exploration enemies and restores the HUD / path state.
void StopConversation(void) {
    if (lbl_805FB0F4 != NULL) {
        fn_8020A474(*lbl_805FB0F4);
        lbl_805FB0F4 = NULL;
    }
    SessionResetUnk204(gGame + 0x5C8);
    u32 flags = lbl_805FB0F0 & ~2u;
    lbl_805FB0F8 = 0;
    lbl_805FB0E0 = 0;
    lbl_805FB0E8 = 0;
    lbl_805FB0F0 = flags;
    if (lbl_805FB0DC != NULL) lbl_805FB0DC = NULL;
    WasButtonPressed(1);
    WasButtonPressed(0xC);
    WasButtonPressed(0xB);
    u8* player = *(u8**)(gGame + 0xD614);
    if (player != NULL) {
        EventSend(gEventMgr, player, NULL, 0x6877205Cu, 0, 0, 0, 0);
        UnPauseAllExplorationEnemies();
        GameFlowSignal(0x6877205C, 0, 0);
        u8* p = *(u8**)(gGame + 0xD614);
        if (WorldGetPhase(gGame + 0xD5F0) == 2 && PathIsAtJunction(*(PathFollower**)(p + 0xA00)) != 0) {
            fn_800D5C48(*(u8**)(p + 0xA00), 1);
            if (WorldGetPhase(gGame + 0xD5F0) == 2) {
                fn_80016FD0(*(Character**)(gGame + 0xD614), 1);
            } else if (WorldGetPhase(gGame + 0xD5F0) == 1) {
                ConversationHideWidget48();
            }
        } else if (WorldGetPhase(gGame + 0xD5F0) == 1) {
            ConversationHideWidget48();
        }
        if (WorldGetPhase(gGame + 0xD5F0) == 2) ScreenSetsFlush(gScreenSets);
    }
}

// 0x800F6774
// Walks every loaded zone's entity list; for each CIAnimatedProp (type 0xCC05AC21) clears byte
// +0x54 and resets the camera object (slots 0x10, 0x14(0), 0x94, 0x24(world+8 id), 0x50(0)).
void StopUsingAnimatedCamera(void) {
    for (s32 z = 0; z <= *(s32*)(*(u8**)gWorldState + 0x4); z++) {
        if (ZoneIsLoaded(gWorldState, z)) {
            u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + z * gZoneResourceSize;
            for (s32 i = 0; i < *(u8*)(zone + 0xAC); i++) {
                u8* ent = *(u8**)(*(u8**)(zone + 0xA8) + i * 4);
                if (*(u32*)(ent + 0x4) == 0xCC05AC21u) {  // "CIAnimatedProp"
                    *(u8*)(ent + 0x54) = 0;
                    u8* cam = *(u8**)(lbl_803C4298 + 4);
                    (*(void (**)(void*))(*(u8**)cam + 0x10))(cam);
                    cam = *(u8**)(lbl_803C4298 + 4);
                    (*(void (**)(void*, s32))(*(u8**)cam + 0x14))(cam, 0);
                    cam = *(u8**)(lbl_803C4298 + 4);
                    (*(void (**)(void*))(*(u8**)cam + 0x94))(cam);
                    cam = *(u8**)(lbl_803C4298 + 4);
                    (*(void (**)(void*, s32))(*(u8**)cam + 0x24))(cam, *(s32*)(*(u8**)(*(u8**)gWorldState + 0x8) + 0x4));
                    cam = *(u8**)(lbl_803C4298 + 4);
                    (*(void (**)(void*, s32))(*(u8**)cam + 0x50))(cam, 0);
                }
            }
        }
    }
}

// 0x800F65E8
// Switches the camera object to mode 6 and aims it along the vector from target+0x80 to
// target+0x98: positions it 0.8 units behind the start point and sets its blend time to 2.0.
void TransitionToRopeCamera(void) {
    u8* cam = *(u8**)(lbl_803C4298 + 4);
    (*(void (**)(void*, s32))(*(u8**)cam + 0xC))(cam, 6);
    u8* t = *(u8**)(lbl_803C4298 + 8);
    f32 dx = *(f32*)(t + 0x98) - *(f32*)(t + 0x80);
    f32 dy = *(f32*)(t + 0x9C) - *(f32*)(t + 0x84);
    f32 dz = *(f32*)(t + 0xA0) - *(f32*)(t + 0x88);
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 dir[3];
    if (lbl_805FEB0C == len2) {
        dir[0] = dir[1] = dir[2] = lbl_805FEB0C;
    } else {
        f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
        dir[0] = dx * inv;
        dir[1] = dy * inv;
        dir[2] = dz * inv;
    }
    f32 k = lbl_805FEB80;
    f32 back[3] = {dir[0] * k, dir[1] * k, dir[2] * k};
    f32 pos[3];
    pos[0] = *(f32*)(t + 0x80) - back[0];
    pos[1] = *(f32*)(t + 0x84) - back[1];
    pos[2] = *(f32*)(t + 0x88) - back[2];
    cam = *(u8**)(lbl_803C4298 + 4);
    // Slot 0x2C also receives two Vec3 by value on the stack: `back` then `dir`.
    (*(void (**)(void*, f32*, u8*, const f32*, const f32*))(*(u8**)cam + 0x2c))(cam, pos, t + 0x98, back, dir);
    cam = *(u8**)(lbl_803C4298 + 4);
    (*(void (**)(void*, f32))(*(u8**)cam + 0x30))(cam, lbl_805FEB48);
}

// 0x800747F4
// Maps a castle experiment name hash to its experiment index (-1 if unknown) and stores it in the
// lab (tail call fn_802188A8).
void SetNextExperiment(u32 a0) {
    s32 idx = -1;
    switch (a0) {
        case 0x011581D3u: idx = 6; break;   // "castle_astral_reach"
        case 0xB4F82008u: idx = 8; break;   // (hash not resolved)
        case 0x9A1B7746u: idx = -1; break;  // "castle_empty"
        case 0x8BD3CCCEu: idx = 10; break;  // "castle_mystic_brew"
        case 0xA3D4805Au: idx = 0; break;   // "castle_brain_toss"
        case 0xC9ABAC90u: idx = 6; break;   // (hash not resolved)
        case 0xB7AEEBD1u: idx = 7; break;   // "castle_brazier"
        case 0x00F7B53Fu: idx = 5; break;   // "castle_spirit_drums"
        case 0x5AE4B3A6u: idx = 4; break;   // "castle_welding"
        case 0x0DA38580u: idx = 2; break;   // "castle_reactor"
        case 0x053BCBA4u: idx = 11; break;  // "castle_grow_juice"
        case 0x5AADD230u: idx = 1; break;   // "castle_saw"
        case 0x7A150909u: idx = 3; break;   // "castle_robot_evolution"
        case 0x5B291F8Cu: idx = 9; break;   // "castle_arm_wrestle"
        default: break;
    }
    fn_802188A8((u8*)gLab, (u8*)(intptr_t)idx);  // r4 is an int index here
}

// 0x8013E780
// Fills the 0x3F event payload with the current monster: name string, four floats, and for each
// of the 5 part slots (stride 0x10 from +0x24) the part def index, tier, quality, enhancement
// and defect (-1 / 0 when the slot is empty).
void TransmitMonster(void) {
    u8* ev = (u8*)EventPost(0x3F, NULL);
    if (ev == NULL) return;
    strcpy((char*)(ev + 0x14), lbl_803F2210);
    *(f32*)(ev + 0x4) = lbl_805FB3F0;
    *(f32*)(ev + 0x8) = lbl_805FB3F4;
    *(f32*)(ev + 0xC) = lbl_805FB3F8;
    *(f32*)(ev + 0x10) = lbl_805FB3FC;
    s32 idx = fn_801138F8();
    lbl_805F90A0 = idx;
    u8* mon = *(u8**)(lbl_805FBBE0 + 0xA30 + idx * 4);
    fn_8013E454(mon, lbl_805FBBE0);
    *(u8**)(lbl_805FBBE0 + 0x114) = mon;
    for (s32 i = 0; i < 5; i++) {
        u8* part = *(u8**)(mon + 4 + i * 4);
        u8* o = ev + i * 0x10;
        if (part == NULL) {
            *(s32*)(o + 0x24) = -1;
            *(u8*)(o + 0x28) = 0;
            *(u8*)(o + 0x29) = 0;
            *(s32*)(o + 0x2C) = -1;
            *(s32*)(o + 0x30) = -1;
        } else {
            *(s32*)(o + 0x24) = *(s32*)(part + 0x0);
            *(u8*)(o + 0x28) = PartGetTier((PartInstance*)*(u8**)(mon + 4 + i * 4));
            *(s8*)(o + 0x29) = PartGetQuality((PartInstance*)*(u8**)(mon + 4 + i * 4));
            *(s32*)(o + 0x2C) = *(s32*)(*(u8**)(mon + 4 + i * 4) + 0x8);
            *(s32*)(o + 0x30) = *(s32*)(*(u8**)(mon + 4 + i * 4) + 0x4);
        }
    }
}

// 0x8001EE48
// Creates the troublescope state (0x3C bytes) once: current monster, its index in the save's
// monster list (-1 if absent), the monster count and the "%d/%d" counter text.
void Troublescope_Init(void) {
    if (lbl_805FAF80 != NULL) return;
    u8* t = (u8*)fn_801D40E0(0x3C);
    if (t != NULL) *(s32*)(t + 0x4) = 0;
    lbl_805FAF80 = t;
    *(s32*)(t + 0x8) = 0;
    *(s32*)(lbl_805FAF80 + 0xC) = 0;
    *(s32*)(lbl_805FAF80 + 0x1C) = 0;
    *(s32*)(lbl_805FAF80 + 0x20) = 0;
    u8* cur = SaveGetCurrentMonster(lbl_805FBBE0);
    *(u8**)(lbl_805FAF80 + 0x4) = cur;
    u8* st = lbl_805FAF80;
    s32 count = *(s32*)(lbl_805FBBE0 + 0x10E8);
    s32 found = -1;
    cur = *(u8**)(st + 0x4);
    for (s32 i = 0; i < count; i++) {
        if (*(u8**)(lbl_805FBBE0 + 0xA30 + i * 4) == cur) { found = i; break; }
    }
    *(s32*)(st + 0x10) = found;
    *(s32*)(lbl_805FAF80 + 0x14) = *(s32*)(lbl_805FAF80 + 0x10);
    lbl_805FAF84 = 0;
    *(s32*)(lbl_805FAF80 + 0x18) = *(s32*)(lbl_805FBBE0 + 0x10E8);
    u8* s = lbl_805FAF80;
    SysSprintf((char*)(s + 0x34), lbl_805F828C, *(s32*)(s + 0x10) + 1, *(s32*)(s + 0x18));
    fn_8001F6A4(lbl_805FAF80);
}

// 0x8006ED68
// Shows a proximity text effect (hash a1) on entity a0: posts event 0xB (unless the entity is
// flagged 0x4000000), spawns the effect at the entity's anchor (+0x194) once, and copies the
// entity's tint vector (+0x1A0) onto the effect's children (a unit vector (0,0,1,0) when the
// tint is within 0.0001 of zero).
void TriggerShowProximityText(u8* arg0, s32 arg1) {
    if (arg1 == 0) return;
    if (!(*(s32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x04000000)) {
        u8* ev = (u8*)EventPost(0xB, arg0);
        if (ev != NULL) *(s32*)(ev + 0x4) = arg1;
    }
    if (*(s32*)(arg0 + 0x178) != 0) return;
    ScopedLock lock;
    ScopedLockCtor(&lock);
    f32 pos[4];
    pos[0] = *(f32*)(arg0 + 0x194);
    pos[1] = *(f32*)(arg0 + 0x198);
    pos[2] = *(f32*)(arg0 + 0x19C);
    pos[3] = lbl_805FD598;
    SpawnEffectAt(lbl_8047C760, (u32)arg1, pos, (u8**)(arg0 + 0x178));
    u8* fx = *(u8**)(arg0 + 0x178);
    if (fx != NULL) {
        f32 c0 = *(f32*)(arg0 + 0x1A0);
        f32 c1 = *(f32*)(arg0 + 0x1A4);
        f32 c2 = *(f32*)(arg0 + 0x1A8);
        f32 c3 = lbl_805FD598;
        f32 hi = lbl_805FD5A0;   // +0.0001
        f32 lo = lbl_805FD5A4;   // -0.0001
        BOOL nearZero = (c0 <= hi && c0 >= lo && c1 <= hi && c1 >= lo && c2 <= hi && c2 >= lo &&
                         c3 <= hi && c3 >= lo);
        if (nearZero) {
            for (s32 i = 0; i < *(s32*)(fx + 0xC); i++) {
                u8* ch = *(u8**)(fx + 0x14 + i * 4);
                if (ch != NULL) {
                    *(f32*)(ch + 0x30) = lbl_805FD598;
                    *(f32*)(ch + 0x34) = lbl_805FD598;
                    *(f32*)(ch + 0x38) = lbl_805FD5A8;
                    *(f32*)(ch + 0x3C) = lbl_805FD598;
                }
            }
        } else {
            for (s32 i = 0; i < *(s32*)(fx + 0xC); i++) {
                u8* ch = *(u8**)(fx + 0x14 + i * 4);
                if (ch != NULL) {
                    *(f32*)(ch + 0x30) = c0;
                    *(f32*)(ch + 0x34) = c1;
                    *(f32*)(ch + 0x38) = c2;
                    *(f32*)(ch + 0x3C) = c3;
                }
            }
        }
    }
    ScopedLockDtor(&lock, -1);
}

// 0x800E8B94
// Places entity a0 (flagged 0x2000, when gGame+0xFE0 is 0) at node a2 of the object db: moves it
// there, clears its velocity, sets mode 3 and refreshes it with a1, and registers a spawn record
// (a3/a4 as floats) when both are non-negative in phase 2.
void SpawnRandomMonsterAtNode(void* a0, u32 a1, u32 a2, s32 a3, s32 a4) {
    u8* e = (u8*)a0;
    u8 ref[0x404];
    u8 rec[0x28];
    if (!(*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x2000) || *(s32*)(gGame + 0xFE0) != 0) return;
    *(s32*)(*(u8**)(e + 0x4) + 0x20) |= 0x100;
    *(s32*)(*(u8**)(e + 0x4) + 0x20) &= 0xFFFFFF7F;
    *(s32*)(*(u8**)(e + 0x4) + 0x28) = 1;
    u8* src = ObjDbLookup(gObjDb, (s32)a2);
    memcpy(ref, src, 0x404);
    Vec3* pos = PathGetPosition((PathFollower*)ref);
    f32 x = pos->x, y = pos->y, z = pos->z;
    EventSend(gEventMgr, e, NULL, 0x337AADAAu, 0, 0, 1, 0);
    *(f32*)(e + 0xC8) = x;
    *(f32*)(e + 0xCC) = y;
    *(f32*)(e + 0xD0) = z;
    *(f32*)(e + 0xD4) = lbl_805FEB0C;
    *(f32*)(e + 0xD8) = lbl_805FEB0C;
    *(f32*)(e + 0xDC) = lbl_805FEB0C;
    u8* st = *(u8**)(e + 0x4);
    *(f32*)(st + 0x0) = x;
    *(f32*)(st + 0x4) = y;
    *(f32*)(st + 0x8) = z;
    fn_80053418(e);
    *(s32*)(e + 0x880) = 3;
    if ((*(s32 (**)(void*))(*(u8**)e + 0xF0))(e) == 0) fn_8004D7E0(e, (s32)a1, 0);
    for (s32 i = 0; i < 5; i++) {
        PartInstance* part = *(PartInstance**)(e + 0x89C + i * 4);
        if (part != NULL) PartGetCurrentHealth(part);
    }
    if (WorldGetPhase(gGame + 0xD5F0) == 2 && a3 >= 0 && a4 >= 0) {
        fn_800D6B0C((s32*)rec);
        *(s32*)(rec + 0x0) = *(s32*)(e + 0x2C);
        *(u32*)(rec + 0x18) = a1;
        *(s32*)(rec + 0x1C) = -1;
        *(s32*)(rec + 0x8) = 0;
        *(u32*)(rec + 0x4) = a2;
        *(f32*)(rec + 0xC) = (f32)a3;
        *(f32*)(rec + 0x10) = (f32)a4;
        *(u8*)(rec + 0x20) = 0;
        fn_800D70EC((s32*)rec, (f32)a3);
    }
    ObjRefDtor(ref, -1);
}

// 0x800E8A6C
// Spawns a random monster at a node of group a2 (looked up via fn_800D4730), returning the node
// id (0 when nothing was spawned); registers a spawn record like SpawnRandomMonsterAtNode.
u32 SpawnRandomMonsterAtNodeGroup(void* a0, u32 a1, s32 a2, s32 a3, s32 a4) {
    u8* e = (u8*)a0;
    u8 rec[0x28];
    s32 node = 0;
    if ((*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x10) && *(s32*)(gGame + 0xFE0) == 0) {
        node = fn_800D4730(gObjDb, a2);
        if (node != 0) {
            SpawnRandomMonsterAtNode(e, a1, (u32)node, a3, a4);
            if (WorldGetPhase(gGame + 0xD5F0) == 2 && a3 >= 0 && a4 >= 0) {
                fn_800D6B0C((s32*)rec);
                *(s32*)(rec + 0x0) = *(s32*)(e + 0x2C);
                *(s32*)(rec + 0x1C) = -1;
                *(u32*)(rec + 0x18) = a1;
                *(s32*)(rec + 0x8) = a2;
                *(f32*)(rec + 0xC) = (f32)a3;
                *(f32*)(rec + 0x10) = (f32)a4;
                *(u8*)(rec + 0x20) = 0;
                fn_800D70EC((s32*)rec, (f32)a3);
            }
        }
    }
    return (u32)node;
}

// 0x800E98C8
// Spawns effect a2 at a child locator of entity a0. The spec a1 selects the locator: u16 flags at
// +0 (bit 0: bone index in byte +2, bit 1: entity locator-table index in byte +2).
void SpawnParticleAtChild(void* a0, u32 a1, u32 a2) {
    u8 loc[0x30];
    if (GetChildLocator48((u8*)a0, (u8*)(uintptr_t)a1, loc)) {
        f32 pos[4];
        pos[0] = *(f32*)(loc + 0x24);
        pos[1] = *(f32*)(loc + 0x28);
        pos[2] = *(f32*)(loc + 0x2C);
        pos[3] = lbl_805FEB0C;
        SpawnEffectAt(lbl_8047C760, a2, pos, 0);
    }
}

// 0x800E9C2C
// Spawns effect a1 at a random point on a random bone segment of entity a0: picks a bone index
// (rand % count), walks back to the nearest earlier bone whose flag bit 0 is set (else bone 0)
// and interpolates between the two positions with a uniform random fraction.
void SpawnParticleOnSkeleton(void* a0, u32 a1) {
    u8* e = (u8*)a0;
    if (*(u8**)(e + 0xA0) == NULL) return;
    s32 count = *(s32*)(**(u8***)(e + 0xA0) + 0x8);
    s32 idx = rand() % count;
    s32 parent = idx - 1;
    s32 off = parent * 8;
    s32 ctr = parent;
    u8* bones = **(u8***)(e + 0xA0);
    if (parent > 0) {
        for (;;) {
            u8* ent = *(u8**)(bones + 0xC) + off;
            if (*(u32*)(ent + 0x4) & 1) break;
            off -= 8;
            parent -= 1;
            ctr -= 1;
            if (ctr == 0) { parent = 0; break; }
        }
    } else {
        parent = 0;
    }
    u8 loc[0x30];
    fn_801B55F4(e, loc, idx);
    f32 a[3] = {*(f32*)(loc + 0x24), *(f32*)(loc + 0x28), *(f32*)(loc + 0x2C)};
    fn_801B55F4(e, loc, parent);
    f32 b[3] = {*(f32*)(loc + 0x24), *(f32*)(loc + 0x28), *(f32*)(loc + 0x2C)};
    f32 t = lbl_805FEB0C + lbl_805FEB04 * (lbl_805FEB1C * (f32)RandomU32());   // 0 + 1.0 * (2^-32 * r)
    f32 d0 = b[0] - a[0];
    f32 d1 = b[1] - a[1];
    f32 d2 = b[2] - a[2];
    f32 pos[4];
    pos[0] = a[0] + d0 * t;
    pos[1] = a[1] + d1 * t;
    pos[2] = a[2] + d2 * t;
    pos[3] = lbl_805FEB0C;
    SpawnEffectAt(lbl_8047C760, a1, pos, 0);
}

// 0x800EA3F8
// Spawns a projectile (object a2 via SpawnHelper) at a child locator of entity a0 (spec a1, as in
// SpawnParticleAtChild), aimed by fn_800E9E54. For two special projectile hashes the velocity is
// scaled by a lob speed computed from the target when the shooter has the matching flag.
void SpawnProjectileAtChild(void* a0, u32 a1, u32 a2, u32 a3) {
    u8* e = (u8*)a0;
    u8 loc[0x30];
    if (!GetChildLocator48(e, (u8*)(uintptr_t)a1, loc)) return;
    u8* proj = (u8*)SpawnHelper((void*)(uintptr_t)a2, 0);
    if (proj == NULL) return;
    f32 vel[4];
    fn_800E9E54(vel, e, loc, a3);
    if (a3 == 0xCEFC060Eu && (*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x20)) {
        if (*(s32*)(e + 0xA08) & 8) {
            u8* d = *(u8**)(proj + 0x228);
            f32 speed = ComputeLobSpeed((const Vec3*)(loc + 0x24), (const Vec3*)(e + 0xB88), lbl_805FEB38, -*(f32*)(d + 0xD4));
            if (speed > lbl_805FEB0C) {
                vel[0] *= speed;
                vel[1] *= speed;
                vel[2] *= speed;
                fn_80065200(proj, loc + 0x24, vel, e, 0, 0x4000, 0x4000);
                return;
            }
        }
    } else if (a3 == 0xC2C3D0ACu && (*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x40)) {
        u8* tgt = *(u8**)(e + 0x18C);
        f32 speed = ComputeLobSpeed((const Vec3*)(loc + 0x24), *(const Vec3**)(tgt + 0x4), lbl_805FEB38,
                                    -*(f32*)(*(u8**)(proj + 0x228) + 0xD4));
        if (speed > lbl_805FEB0C) {
            vel[0] *= speed;
            vel[1] *= speed;
            vel[2] *= speed;
            fn_80065200(proj, loc + 0x24, vel, e, 0, 0x4000, 0x4000);
            return;
        }
    }
    fn_80065200(proj, loc + 0x24, vel, e, 0, 0, 0);
}

// 0x801EDF58
// Resolves a node reference (4 bytes: kind in the low nibble, bit 7 = "by id") to a value:
//   kind 1: id of an entity (from the ref or the global lbl_805FB6B8) -> u16 at +0x30 of its state
//   kind 4: world zone table entry (u16 index at +2) -> word at +4
//   other:  world position lookup through fn_801ED788 + fn_8012EE30
// The reference word is read as a whole so the code does not depend on byte order.
s32 fn_801EDF58(u8* ref) {
    u32 w = NodeRefWord(ref);
    u32 kind = NodeRefKind(w);
    if (kind == 4) {
        u32 idx = NodeRefKind(w);
        idx = (idx == 4) ? NodeRefHalf2(w) : 0xFFFFFFFFu;
        u8* w = *(u8**)gWorldState;
        u8* zone = (idx < *(u32*)(w + 0x24)) ? *(u8**)(w + 0x28) + idx * 32 : NULL;
        if (zone == NULL) return 0;
        return *(s32*)(zone + 4);
    }
    if (kind == 1) {
        s32 id = lbl_805FB6B8;
        if (NodeRefIsId(w)) id = (s32)w;
        u8* obj = (u8*)fn_801B7198(&id, 0);
        if (obj == NULL) return 0;
        u8* st = *(u8**)(obj + 4);
        if (st == NULL) return 0;
        return *(u16*)(st + 0x30);
    }
    f32 pos[3];
    fn_801ED788(pos, ref);
    return fn_8012EE30(gWorldState, pos);
}

// 0x800EC89C
// Teleports entity a0 to node a1 (event 0xF with the node and the delay a2 posted first). When
// a2 >= 0 the entity is moved immediately: state position set, update flags raised, child object
// notified, and for flagged (0x10) entities the saved position / velocity fields are reset.
void TeleportTo(void* a0, u32 a1, f32 a2) {
    u8* e = (u8*)a0;
    u8* node = (u8*)(uintptr_t)a1;
    u8* ev = (u8*)EventPost(0xF, e);
    if (ev != NULL) {
        *(s32*)(ev + 0x4) = *(s32*)node;
        *(f32*)(ev + 0x8) = a2;
    }
    if (a2 >= lbl_805FEB0C) {
        f32 pos[3];
        fn_801ED788(pos, node);
        u8* st = *(u8**)(e + 0x4);
        *(f32*)(st + 0x0) = pos[0];
        *(f32*)(st + 0x4) = pos[1];
        *(f32*)(st + 0x8) = pos[2];
        // The original ORs these flag bits in repeatedly (0x8000, 0x10000000, 0x4000, 0x10000000, ...).
        for (s32 i = 0; i < 2; i++) {
            *(s32*)(*(u8**)(e + 0x4) + 0x20) |= 0x8000;
            *(s32*)(*(u8**)(e + 0x4) + 0x20) |= 0x10000000;
            *(s32*)(*(u8**)(e + 0x4) + 0x20) |= 0x4000;
            *(s32*)(*(u8**)(e + 0x4) + 0x20) |= 0x10000000;
        }
        u8* child = *(u8**)(e + 0x8C);
        if (child != NULL) fn_801B1778(child, 1);
        if (*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x10) {
            *(f32*)(e + 0x74C) = pos[0];
            *(f32*)(e + 0x750) = pos[1];
            *(f32*)(e + 0x754) = pos[2];
            *(f32*)(e + 0x16C) = a2;
            *(f32*)(e + 0x844) = lbl_805FEB0C;
            *(f32*)(e + 0x848) = lbl_805FEB0C;
            *(f32*)(e + 0x84C) = lbl_805FEB0C;
            *(f32*)(e + 0x850) = lbl_805FEB0C;
            *(f32*)(e + 0x854) = lbl_805FEB0C;
            *(f32*)(e + 0x858) = lbl_805FEB0C;
        }
    }
}

// 0x800EC45C
// Starts laser a1 (slot 0 or 1) on entity a0: posts event 0x27 (unless flagged 0x4000000), then
// for player-type entities (state flag 0x40 uses the global slots lbl_805FB1D8/1E0, flag 0x20
// uses the slots in the entity at +0xC2C/+0xC34) stores the target word *a2 and lazily
// creates the laser effect (fn_801C3EA4 with a3), then starts it (fn_801C5C60).
void StartLaser(void* a0, s32 a1, u32 a2, u32 a3) {
    u8* e = (u8*)a0;
    if (!(*(s32*)(*(u8**)(e + 0x4) + 0x20) & 0x04000000)) {
        u8* ev = (u8*)EventPost(0x27, e);
        if (ev != NULL) {
            *(s32*)(ev + 0x4) = a1;
            *(s32*)(ev + 0x8) = *(s32*)(uintptr_t)a2;
            *(u32*)(ev + 0xC) = a3;
        }
    }
    s32 flags = *(s32*)(*(u8**)(e + 0x4) + 0x1C);
    if (flags & 0x40) {
        if (a1 >= 0 && a1 < 2) {
            s32 had = lbl_805FB1E0[a1];
            lbl_805FB1D8[a1] = *(s32*)(uintptr_t)a2;
            if (had == 0) lbl_805FB1E0[a1] = (s32)(intptr_t)fn_801C3EA4(lbl_8047C7AC, (s32)a3);  // lint:ok
            fn_801C5C60(lbl_805FB1E0[a1]);
        }
    } else if (flags & 0x20) {
        if (a1 >= 0 && a1 < 2) {
            u8* slot = e + a1 * 4;
            *(s32*)(slot + 0xC2C) = *(s32*)(uintptr_t)a2;
            if (*(s32*)(slot + 0xC34) == 0) *(s32*)(slot + 0xC34) = (s32)(intptr_t)fn_801C3EA4(lbl_8047C7AC, (s32)a3);  // lint:ok
            fn_801C5C60(*(s32*)(slot + 0xC34));
        }
    }
}

// 0x800E8E00
// Places entity a0 at node a1 of the object db (phase 2 only, when gGame+0xFE0 is 0): moves it
// there, clears its velocity and registers a spawn record (a2/a3 as floats, when both >= 0).
// Flagged (0x10) entities also get byte +0x6B8 set, path-length field +0x9D8 and (for one
// particular type) their path paused.
void SpawnMonsterAtNode(void* a0, u32 a1, s32 a2, s32 a3) {
    u8* e = (u8*)a0;
    u8 ref[0x404];
    u8 rec[0x28];
    if (WorldGetPhase(gGame + 0xD5F0) != 2) return;
    if (*(s32*)(gGame + 0xFE0) != 0) return;
    u8* src = ObjDbLookup(gObjDb, (s32)a1);
    memcpy(ref, src, 0x404);
    Vec3* pos = PathGetPosition((PathFollower*)ref);
    f32 x = pos->x, y = pos->y, z = pos->z;
    *(s32*)(*(u8**)(e + 0x4) + 0x20) |= 0x100;
    *(s32*)(*(u8**)(e + 0x4) + 0x20) &= 0xFFFFFF7F;
    *(s32*)(*(u8**)(e + 0x4) + 0x28) = 1;
    EventSend(gEventMgr, e, NULL, 0x337AADAAu, 0, 0, 1, 0);
    *(f32*)(e + 0xC8) = x;
    *(f32*)(e + 0xCC) = y;
    *(f32*)(e + 0xD0) = z;
    *(f32*)(e + 0xD4) = lbl_805FEB0C;
    *(f32*)(e + 0xD8) = lbl_805FEB0C;
    *(f32*)(e + 0xDC) = lbl_805FEB0C;
    u8* st = *(u8**)(e + 0x4);
    *(f32*)(st + 0x0) = x;
    *(f32*)(st + 0x4) = y;
    *(f32*)(st + 0x8) = z;
    if (*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x10) {
        *(u8*)(e + 0x6B8) = 1;
        fn_80053418(e);
        if (fn_800D59D4(ref) > 0) *(s32*)(e + 0x9D8) = fn_800D59D4(ref);
        if (*(u32*)(e + 0x6B4) == 0x0957DD45u) PathSetPaused(*(PathFollower**)(e + 0xA00), 1);
    }
    if ((*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x2000) &&
        (*(s32 (**)(void*))(*(u8**)e + 0xF0))(e) == 0) {
        fn_8004D968(e, 1, 0);
    }
    if (WorldGetPhase(gGame + 0xD5F0) == 2 && a2 >= 0 && a3 >= 0) {
        fn_800D6B0C((s32*)rec);
        *(s32*)(rec + 0x0) = *(s32*)(e + 0x2C);
        *(s32*)(rec + 0x1C) = -1;
        *(s32*)(rec + 0x18) = -1;
        *(s32*)(rec + 0x8) = 0;
        *(u32*)(rec + 0x4) = a1;
        *(f32*)(rec + 0xC) = (f32)a2;
        *(f32*)(rec + 0x10) = (f32)a3;
        *(u8*)(rec + 0x20) = 0;
        fn_800D70EC((s32*)rec, (f32)a2);
    }
    ObjRefDtor(ref, -1);
}

// 0x800EE768
// Spawns a lightning bolt between two node refs a0 / a1 (both must resolve to the same zone and
// be valid) with width a2 and effect hash a3 (default 0x62AECFDE). The owner object is the first
// ref that is a live entity (kind 1).
void SpawnLightningBetweenNodes(u32 a0, u32 a1, f32 a2, u32 a3) {
    u8* n0 = (u8*)(uintptr_t)a0;
    u8* n1 = (u8*)(uintptr_t)a1;
    u32 hash = a3;
    s32 z1 = fn_801EDF58(n1);
    s32 z0 = fn_801EDF58(n0);
    if (z0 != z1) return;
    if (hash == 0) hash = 0x62AECFDE;
    if (fn_801ED368(n0) == 0) return;
    if (fn_801ED368(n1) == 0) return;
    s32 idx = fn_801EDF58(n0);
    u8* owner = NULL;
    if (fn_801ED2FC(n0) == 1) {
        s32 id = fn_801ED538(n0);
        owner = (u8*)fn_801B7198(&id, 0);
        idx = *(u16*)(*(u8**)(owner + 4) + 0x32);
    } else if (fn_801ED2FC(n1) == 1) {
        s32 id = fn_801ED538(n1);
        owner = (u8*)fn_801B7198(&id, 0);
        idx = *(u16*)(*(u8**)(owner + 4) + 0x32);
    }
    s32 v1 = *(s32*)n1;
    s32 v0 = *(s32*)n0;
    u8 d[0x80];
    u8 out[0x28];
    memset(d, 0, 0x64);
    *(f32*)(d + 0x70) = lbl_805FEB0C;
    *(u16*)(d + 0x78) = 0xFFFF;
    *(u16*)(d + 0x7A) = 0xFFFF;
    *(u16*)(d + 0x7C) = 0xFFFF;
    *(s32*)(d + 0x74) = 0;
    *(u8*)(d + 0x7E) = 0;
    *(u8*)(d + 0x7F) = 0;
    *(s32*)(d + 0x68) = 0;
    *(void**)(d + 0x6C) = (void*)fn_800ED944;   // callback slot (32-bit on the original)
    *(u32*)(d + 0x48) = hash;
    *(s32*)(d + 0x4C) = 0;
    *(s32*)(d + 0x08) = v0;
    *(s32*)(d + 0x0C) = v1;
    *(f32*)(d + 0x10) = lbl_805FEB0C;
    *(f32*)(d + 0x14) = lbl_805FEB0C;
    *(f32*)(d + 0x18) = lbl_805FEB04;
    *(f32*)(d + 0x1C) = lbl_805FEB0C;
    *(f32*)(d + 0x20) = lbl_805FEB0C;
    *(f32*)(d + 0x24) = lbl_805FEB0C;
    *(s32*)(d + 0x40) = 0;
    *(s32*)(d + 0x44) = 0;
    *(s32*)(d + 0x00) = 0;
    *(s32*)(d + 0x04) = 0;
    *(f32*)(d + 0x3C) = lbl_805FEB0C;
    *(f32*)(d + 0x28) = (a2 >= lbl_805FEB0C) ? a2 : lbl_805FEB0C;
    *(f32*)(d + 0x2C) = lbl_805FEB0C;
    *(f32*)(d + 0x30) = lbl_805FEB0C;
    *(f32*)(d + 0x34) = lbl_805FEB0C;
    *(f32*)(d + 0x38) = lbl_805FEB0C;
    *(s32*)(d + 0x50) = ((a2 < lbl_805FEB0C) ? 1 : 0) | 8;
    *(u32*)(d + 0x64) = LightningHash48();
    fn_801E8D74(out, *(u8**)(*(u8**)gWorldState + 0x1C8) + idx * 0xC, owner, d);
}

// 0x800ED6AC
// Like SpawnLightningBetweenNodes, with a random-bolt width a2 and two extra parameters a3 / a4
// stored in the descriptor at +0x34 / +0x38; hash a5 (default 0x62AECFDE).
void SpawnRandomLightningBetweenNodes(u32 a0, u32 a1, f32 a2, f32 a3, f32 a4, u32 a5) {
    u8* n0 = (u8*)(uintptr_t)a0;
    u8* n1 = (u8*)(uintptr_t)a1;
    u32 hash = a5;
    s32 z1 = fn_801EDF58(n1);
    s32 z0 = fn_801EDF58(n0);
    if (z0 != z1) return;
    if (hash == 0) hash = 0x62AECFDE;
    if (fn_801ED368(n0) == 0) return;
    if (fn_801ED368(n1) == 0) return;
    s32 idx = fn_801EDF58(n0);
    u8* owner = NULL;
    if (fn_801ED2FC(n0) == 1) {
        s32 id = fn_801ED538(n0);
        owner = (u8*)fn_801B7198(&id, 0);
        idx = *(u16*)(*(u8**)(owner + 4) + 0x32);
    } else if (fn_801ED2FC(n1) == 1) {
        s32 id = fn_801ED538(n1);
        owner = (u8*)fn_801B7198(&id, 0);
        idx = *(u16*)(*(u8**)(owner + 4) + 0x32);
    }
    s32 v1 = *(s32*)n1;
    s32 v0 = *(s32*)n0;
    u8 d[0x80];
    u8 out[0x28];
    memset(d, 0, 0x64);
    *(f32*)(d + 0x70) = lbl_805FEB0C;
    *(u16*)(d + 0x78) = 0xFFFF;
    *(u16*)(d + 0x7A) = 0xFFFF;
    *(u16*)(d + 0x7C) = 0xFFFF;
    *(s32*)(d + 0x74) = 0;
    *(u8*)(d + 0x7E) = 0;
    *(u8*)(d + 0x7F) = 0;
    *(s32*)(d + 0x68) = 0;
    *(void**)(d + 0x6C) = (void*)fn_800ED944;   // callback slot (32-bit on the original)
    *(u32*)(d + 0x48) = hash;
    *(s32*)(d + 0x4C) = 0;
    *(s32*)(d + 0x08) = v0;
    *(s32*)(d + 0x0C) = v1;
    *(f32*)(d + 0x10) = lbl_805FEB0C;
    *(f32*)(d + 0x14) = lbl_805FEB0C;
    *(f32*)(d + 0x18) = lbl_805FEB04;
    *(f32*)(d + 0x1C) = lbl_805FEB0C;
    *(f32*)(d + 0x20) = lbl_805FEB0C;
    *(f32*)(d + 0x24) = lbl_805FEB0C;
    *(s32*)(d + 0x40) = 0;
    *(s32*)(d + 0x44) = 0;
    *(s32*)(d + 0x00) = 0;
    *(s32*)(d + 0x04) = 0;
    *(f32*)(d + 0x3C) = lbl_805FEB0C;
    *(f32*)(d + 0x28) = (a2 >= lbl_805FEB0C) ? a2 : lbl_805FEB0C;
    *(f32*)(d + 0x2C) = lbl_805FEB0C;
    *(f32*)(d + 0x30) = lbl_805FEB0C;
    *(f32*)(d + 0x34) = lbl_805FEB0C;
    *(f32*)(d + 0x38) = lbl_805FEB0C;
    *(s32*)(d + 0x50) = ((a2 < lbl_805FEB0C) ? 1 : 0) | 8;
    *(u32*)(d + 0x64) = LightningHash48();
    *(f32*)(d + 0x34) = a3;
    *(f32*)(d + 0x38) = a4;
    fn_801E8D74(out, *(u8**)(*(u8**)gWorldState + 0x1C8) + idx * 0xC, owner, d);
}

// 0x800EED80
// Spawns a lightning bolt from a child locator of entity a0 (spec a1) to node ref a2, with width
// a3, hash a4 (default 0x62AECFDE) and extra word a5. The entity is the owner.
void SpawnLightningFromChildToNode(void* a0, u32 a1, u32 a2, f32 a3, u32 a4, u32 a5) {
    u8* e = (u8*)a0;
    u8* spec = (u8*)(uintptr_t)a1;
    u8* node = (u8*)(uintptr_t)a2;
    u32 hash = a4;
    u8 loc[0x30];
    if (e == NULL) return;
    if (!GetChildLocator48(e, spec, loc)) return;
    if (fn_801ED368(node) == 0) return;
    if (hash == 0) hash = 0x62AECFDE;
    s32 nodeVal = *(s32*)node;
    s32 specVal = *(s32*)spec;
    u8 d[0x80];
    u8 out[0x28];
    *(u16*)(d + 0x78) = 0xFFFF;
    *(u16*)(d + 0x7A) = 0xFFFF;
    *(u16*)(d + 0x7C) = 0xFFFF;
    memset(d, 0, 0x64);
    *(s32*)(d + 0x64) = 0;
    *(s32*)(d + 0x68) = 0;
    *(f32*)(d + 0x70) = lbl_805FEB0C;
    *(u16*)(d + 0x78) = 0xFFFF;
    *(u16*)(d + 0x7A) = 0xFFFF;
    *(u16*)(d + 0x7C) = 0xFFFF;
    *(s32*)(d + 0x74) = 0;
    *(u8*)(d + 0x7E) = 0;
    *(u8*)(d + 0x7F) = 0;
    *(void**)(d + 0x6C) = (void*)fn_800ED944;   // callback slot (32-bit on the original)
    *(u32*)(d + 0x48) = hash;
    *(u32*)(d + 0x4C) = a5;
    *(s32*)(d + 0x00) = specVal;
    *(s32*)(d + 0x0C) = nodeVal;
    *(f32*)(d + 0x10) = lbl_805FEB0C;
    *(f32*)(d + 0x14) = lbl_805FEB0C;
    *(f32*)(d + 0x18) = lbl_805FEB04;
    *(f32*)(d + 0x1C) = lbl_805FEB0C;
    *(f32*)(d + 0x20) = lbl_805FEB0C;
    *(f32*)(d + 0x24) = lbl_805FEB0C;
    *(s32*)(d + 0x44) = 0;
    *(f32*)(d + 0x3C) = lbl_805FEB0C;
    *(s32*)(d + 0x04) = 0;
    *(u16*)(d + 0x0A) = 0xFFFF;
    *(u8*)(d + 0x08) = 0;
    *(u8*)(d + 0x09) = 0;
    *(f32*)(d + 0x28) = (a3 >= lbl_805FEB0C) ? a3 : lbl_805FEB0C;
    *(f32*)(d + 0x2C) = lbl_805FEB0C;
    *(f32*)(d + 0x30) = lbl_805FEB0C;
    *(f32*)(d + 0x34) = lbl_805FEB0C;
    *(f32*)(d + 0x38) = lbl_805FEB0C;
    *(s32*)(d + 0x40) = 0;
    *(s32*)(d + 0x50) = ((a3 < lbl_805FEB0C) ? 1 : 0) | 4;
    *(u32*)(d + 0x64) = LightningHash48();
    fn_801E8D74(out, *(u8**)(*(u8**)gWorldState + 0x1C8) + *(u16*)(*(u8**)(e + 4) + 0x32) * 0xC, e, d);
}

// 0x80123954
// Fills a save-file header/body buffer (r4 = buf): the slot id (a0), the five options (brightness,
// music, voice, sfx volumes, rumble), two words from fn_800FB444 / fn_800FB45C, a copy of the
// 0xB60-byte settings block, and - when the save has a payload (word +0x924 of the save block) -
// its checksum (+0xB8C) and size (+0xB90), computed over a 32-byte aligned scratch copy built
// from the game-name / title strings.
void fn_80123954(s32 arg0, u8* arg1) {
    char title[0x100];
    char name[0x100];
    *(s16*)(arg1 + 0xB80) = (s16)arg0;
    *(s16*)(arg1 + 0xB88) = (s16)GetOptionsBrightness();
    *(s16*)(arg1 + 0xB84) = (s16)GetOptionsMusicVolume();
    *(s16*)(arg1 + 0xB86) = (s16)GetOptionsVoiceVolume();
    *(s16*)(arg1 + 0xB82) = (s16)GetOptionsSfxVolume();
    *(u8*)(arg1 + 0xB8A) = (u8)GetOptionsRumble();
    *(s32*)(arg1 + 0x0) = fn_800FB444();
    *(s32*)(arg1 + 0x4) = fn_800FB45C();
    *(s32*)(arg1 + 0xB8C) = 0;
    *(s32*)(arg1 + 0xB90) = 0;
    memcpy(arg1 + 0x20, lbl_80556E80, 0xB60);
    u8* tbl = (u8*)gStringTable;
    const char* src;
    if (*(u8**)tbl != NULL && *(u32*)(tbl + 0xC) > 0x5B5u) {
        src = *(const char**)(*(u8**)tbl + 0x16D4);
    } else {
        src = lbl_805F8200;
    }
    strcpy(title, src);
    strcpy(name, lbl_805F8FD4);
    if (*(s32*)(lbl_805FBBE0 + 0x924) != 0) {
        u8* raw = (u8*)MemAllocSmall(0xF0C0);
        u8* aligned = (u8*)(((uintptr_t)raw + 0x1F) & ~(uintptr_t)0x1F);
        fn_80206F14(aligned, title, name);
        *(u32*)(arg1 + 0xB8C) = fn_80143974(0, aligned, *(u32*)(lbl_805FBBE0 + 0x924));
        *(s32*)(arg1 + 0xB90) = *(s32*)(lbl_805FBBE0 + 0x924);
        MemFreeSmall(raw);
    }
}

// 0x801F1D64
// Per-frame tween callback for a bouncing alpha task (ref obj, kind byte at +0x7F):
//   0x20: delegates to fn_801F1EB0;  0x08: returns 2;  0x40: returns 3;  0x10: bounce step:
// reads the current value (from the dispatch-table entry of obj+0 slot 0x30, or from the state
// at obj+4 +0x9C), clamps it into [obj+0xC, obj+0x10], writes it back (slot 0x24 or the state)
// and returns 2 (3 when no target exists); anything else returns 0.
s32 fn_801F1D64(u8* obj, f32 dt) {
    switch (*(u8*)(obj + 0x7F)) {
        case 0x20:
            return fn_801F1EB0(obj, dt);
        case 0x08:
            return 2;
        case 0x10: {
            u8* tgt = *(u8**)obj;
            f32 v;
            if (tgt != NULL) {
                typedef s32 (*GetFn)(void*, s32);
                GetFn get = *(GetFn*)(lbl_80348F38 + *(u8*)(tgt + 0xC) * 4 + 0x30);
                s32 bits = get(*(void**)(tgt + 0x8), 0x10);
                memcpy(&v, &bits, 4);
            } else {
                u8* st = *(u8**)(obj + 4);
                if (st == NULL) return 3;
                v = *(f32*)(st + 0x9C);
            }
            f32 hi = *(f32*)(obj + 0x10);
            f32 lo = *(f32*)(obj + 0xC);
            if (v >= hi) v = hi;
            f32 r = (lo <= v) ? v : lo;
            tgt = *(u8**)obj;
            if (tgt != NULL) {
                typedef void (*SetFn)(void*, s32, s32);
                SetFn set = *(SetFn*)(lbl_80348F38 + *(u8*)(tgt + 0xC) * 4 + 0x24);
                s32 bits;
                memcpy(&bits, &r, 4);
                set(*(void**)(tgt + 0x8), 0x10, bits);
            } else {
                *(f32*)(*(u8**)(obj + 4) + 0x9C) = r;
            }
            return 2;
        }
        case 0x40:
            return 3;
        default:
            return 0;
    }
}

// 0x801EE588
// Blocking system-message screen: shows text a0 (default lbl_805F96C0), one line per "\n"
// separator, centred on the screen, redrawing every frame until the callback a1 returns non-zero.
// Suspends the home-button state lbl_805FBA30 while it runs and restores it afterwards.
void ShowSystemMessage(const char* a0, BOOL (*a1)(void)) {
    s32 savedHome = lbl_805FBA30;
    lbl_805FBA30 = 0;
    const char* msg = a0;
    if (msg == NULL) msg = lbl_805F96C0;
    s32 wasRendering = 0;
    if (lbl_805FB51C != 0) {
        wasRendering = 1;
        fn_80150148();
        fn_801FC0A0();
        fn_801FC1EC();
        RendEndFrame();
    }
    s32 done = 0;
    while (done == 0) {
        char buf[0x400];
        RendBeginFrame();
        RendClear(7, (s32)0xFF000000, 0, lbl_80600078);
        fn_80150148();
        fn_80188ED4(2);
        fn_80188E5C(NULL);
        strncpy(buf, msg, 0x400);
        buf[0x3FF] = 0;
        s32 lineH = lbl_805FB63C;
        s32 lines = 1;
        char* p = buf;
        while (p != NULL) {
            p = fn_80338BC0(p, lbl_805F96C8);
            if (p != NULL) {
                p += 1;
                lines += 1;
            }
        }
        lbl_805FB630 = -1;
        char* cur = buf;
        s32 y = 0;
        for (s32 i = 0; i < lines; i++) {
            char* line = cur;
            cur = fn_80338BC0(line, lbl_805F96C8);
            if (cur != NULL) {
                *cur = 0;
                cur += 1;
            }
            s32 w, h;
            fn_80188EDC(line, &w, &h);
            lbl_805FB628 = (*(s32*)lbl_80412BE0 - w) / 2;
            lbl_805FB62C = y + (*(s32*)(lbl_80412BE0 + 4) / 2 - h);
            fn_80188F00(lbl_805F96CC, line);
            y += lineH;
        }
        fn_80188E5C(NULL);
        fn_80188ED4(0);
        fn_80188E5C(NULL);
        fn_801FC0A0();
        fn_801FC1EC();
        RendEndFrame();
        if (a1() != 0) done = 1;
        fn_80142970();
    }
    if (wasRendering) {
        RendBeginFrame();
        RendClear(7, (s32)0xFF000000, 0, lbl_80600078);
    }
    lbl_805FBA30 = savedHome;
}

// @@APPEND@@
}  // extern "C"
