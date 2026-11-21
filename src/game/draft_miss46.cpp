// Decompiled functions (batch miss46). Verified against the asm.
#include <math.h>
#include <string.h>
#include "types.h"
#include "game/part.h"
#include "engine/engine.h"
#include "engine/rend.h"
extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

// Defined with C++ linkage in save.cpp (SaveGameRead/Write are also passed r6 = 1, unused).
extern "C++" {
extern void* gScreenSets;                                        // 0x805FB654
extern void* gEventMgr;                                          // 0x805FB6C8
BOOL SaveFileExists(const char* dir, const char* file);          // 0x80011960
s32 SaveGameRead(const char* name, void* data, u32 size);        // 0x800113CC
s32 SaveGameWrite(const char* name, const void* data, u32 size); // 0x80011024
void SaveResetBanner(void);                                      // 0x80011018
s32 SaveSetupBanner(void);                                       // 0x800118C4
}

extern "C" {
s32 fn_8006D248(s32);                       // 0x8006D248
s32 fn_8006FA5C(void* obj, s32 flags);      // 0x8006FA5C
s32* fn_800701A4(void);                     // 0x800701A4
u32 fn_80070270(s32 flags);                 // 0x80070270
void fn_8016A43C(u8* p);                    // 0x8016A43C
void fn_8016A788(u8* p);                    // 0x8016A788
BOOL fn_801F56FC(s32 flags);                // 0x801F56FC
BOOL fn_80142358(void);                     // 0x80142358
u32 fn_80142468(void);                      // 0x80142468
extern u8* lbl_805FBBE0;
// Generated leaf functions (src/leaf) and lab_books_f.cpp; they receive the page query result.
s32 fn_80120444(s32 a3);  // 0x80120444
s32 fn_80120474(s32 a3);  // 0x80120474
s32 fn_801204A4(s32 a3);  // 0x801204A4
s32 fn_801204D4(s32 a3);  // 0x801204D4
s32 fn_8011BE84(u8* a3);  // 0x8011BE84
s32 fn_8011BEB0(u8* a3);  // 0x8011BEB0
s32 fn_8011BEDC(u8* a3);  // 0x8011BEDC
s32 fn_8011BF08(u8* a3);  // 0x8011BF08
u32 fn_801203F0(s32 a3);  // 0x801203F0
s32 fn_8011BE4C(u8* p);   // 0x8011BE4C
extern u8* lbl_805FB248;
u8* fn_801E7424(u8* base, u16* id);      // 0x801E7424
BOOL fn_801E7460(u8* base, u16* id);     // 0x801E7460
s32 fn_801E9678(void* list, void* handle);  // 0x801E9678
u32 StringHash(const char* s);
void* CharacterFind(s32 id);
void* EventPost(s32 id, void* sender);
void ScopedLockCtor(ScopedLock* l);
void ScopedLockDtor(ScopedLock* l, s32 flags);
void EulerToQuat(Quat* out, const Vec3* euler);
void __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);  // 0x8032F2E0
void fn_8032F1E8(void* arr, void (*ctor)(void), void (*dtor)(void), s32 size, s32 count);  // 0x8032F1E8
void* fn_8022ED60(void* p);                // 0x8022ED60
u8* fn_8022ED98(u8* p, s32 del);           // 0x8022ED98
void fn_8022F50C(u8* slot);                 // 0x8022F50C
void fn_80123954(s32 v, u8* save);          // 0x80123954
void fn_80142870(s32 a3);                  // 0x80142870
s32 fn_80206DA4(void);                     // 0x80206DA4
s32 fn_80266BA4(const char* name, u8* handle, s32 a);  // 0x80266BA4
s32 fn_80265F80(u8* h, void* buf, u32 n);  // 0x80265F80
s32 fn_80266E24(u8* handle);               // 0x80266E24
s32 fn_80265D78(void* p);                  // 0x80265D78
u32 fn_80143974(s32 seed, void* data, u32 len);  // 0x80143974
void* MemAllocSmall(u32 size);             // 0x801D41CC
void MemFreeSmall(void* p);                // 0x801D4340
void fn_800FB438(s32 a3);                  // 0x800FB438
void fn_800FB450(s32 a3);                  // 0x800FB450
extern s32 lbl_805FB2C8;
extern s32 lbl_805FB2CC;
extern u8 lbl_805FB2D5;
extern const char lbl_805F8FD4[];
extern const char* lbl_805F8FD0;
extern u8 lbl_80556E80[];
extern u8 gWorldState[];
extern u8 gGame[];
extern void* gObjDb;
extern u8 lbl_80348F38[];                    // 0x80348F38 (overlay dispatch table)
extern u8* lbl_805FAF78;                     // monster-builder UI state
u8* LabAllocMonster(u8* inv);
void MonsterSetName(void* monster, const char* name);
s32 fn_80226C3C(u8* save, s32 a);            // 0x80226C3C
s32 fn_801DC430(s32 t);                      // 0x801DC430
s32 fn_801DC3AC(s32 t);                      // 0x801DC3AC
s32 fn_801DC4D4(s32 t);                      // 0x801DC4D4
void* fn_801DA758(void* v);                  // 0x801DA758
s32 WorldGetPhase(void* world);
u8* ObjDbLookup(void* db, s32 id);
u8* ObjRefDtor(u8* ref, s32 flags);
Vec3* PathGetPosition(PathFollower* p);
s32 fn_800D3C60(void* db, void* path, f32 a, f32 b);  // 0x800D3C60
void fn_80053550(u8* ent, s32 a);             // 0x80053550
void fn_80053418(u8* arg0);                  // 0x80053418
u8* fn_8015EC9C(u8* tbl, f32 a, f32 b);      // 0x8015EC9C
extern u8 lbl_8040D500[];
u8* fn_8004EDD0(u8* obj, s32 flag, s32 a2);   // 0x8004EDD0
s32 fn_8004E2A4(u8* fighters, s32 slot);     // 0x8004E2A4
s32 fn_8004E2C4(u8* fighters, s32 slot);     // 0x8004E2C4
u32 fn_8004E2E4(u8* fighters, s32 slot);     // 0x8004E2E4
void fn_8002D508(s32 idx, f32 ratio, u32 extra);  // 0x8002D508
BOOL ZoneIsLoaded(void* world, s32 zone);
void fn_801E8D74(void* out, void* list, void* owner, void* anim);  // 0x801E8D74
s32  fn_801F2018(u8* task, f32 dt);                   // 0x801F2018 (tween update callback)
s32 fn_801F1D64(u8* obj);                    // 0x801F1D64 (tween update callback)
f32 fn_801EDD00(u8* obj);                    // 0x801EDD00
void fn_801ED788(f32* out, u8* a1);          // 0x801ED788
s32 fn_801EDF58(u8* ref);                      // 0x801EDF58
void ScreenSetsActivate(void* sets, u32 hash);       // 0x8018D70C
void* ScreenSetsCurrent(void* sets);                 // 0x8018D1E4
void* ScreenSetFindElement(void* set, u32 hash);     // 0x80198F18
void* ScreenSetFindWidget(void* set, u32 hash);      // 0x80198E34
void* UIWidgetFindChild(void* widget, u32 hash);     // 0x80199AEC
void UISetText(void* el, const char* text, s32 a);   // 0x8019EFC8
void UISetProgress(void* obj, f32 t);
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
void SpawnEffectAt(void* fxMgr, u32 effect, const f32 posAndScale[4], int a);  // 0x801C80E0
u32 fn_801F38F8(u8* a3);                             // 0x801F38F8
void BufferFree(void* p);                            // 0x801D4980
void operator_delete(void* p);                       // 0x801D426C
s32 fn_801E4E20(u8* arg0, s32 arg1);                                // 0x801E4E20
void fn_801E5220(u8* q, s32* handle);                          // 0x801E5220
extern void* gLab;                                   // 0x805FBBC8
extern u8 lbl_805FB0E0;
extern s32 lbl_805FB0B8;
extern u32 lbl_80358B30[];
extern u32 lbl_80358B40[];
extern const char lbl_805F8DA0[];                    // "1 / 1"
extern const char lbl_805F90D4[];
extern u8 lbl_805FB41C;
extern u8 lbl_8047C760[];
extern u8 lbl_8048A8B4[];
void* fn_8004D968(u8*, s32, s32);                      // 0x8004D968
void fn_800D6B0C(s32* rec);                          // 0x800D6B0C (resets a spawn record)
void fn_800D70EC(s32* rec, f32 amount);              // 0x800D70EC (registers a spawn record)
s32 fn_800D4730(void* db, s32 id);                   // 0x800D4730
s32 PartGetCurrentHealth(const PartInstance* p);  // 0x8021EF68
void fn_801499F8(u8* target, s32 w, s32 h, s32 fmt, s32 a, s32 b, const char* name);  // 0x801499F8
void fn_80156F70(s32 idx, s32 a);                    // 0x80156F70
u8* fn_80156AC0(s32 a);                             // 0x80156AC0
void fn_8015D2B8(RendVideo* video, s32 a, void (*cb)(void));  // 0x8015D2B8
void fn_80157494(void);                              // 0x80157494
extern u8 lbl_80401C30[];                            // 0x80401C30 (render target table)
extern const char lbl_803765C0[];                    // 0x803765C0 (target name strings)
extern const char lbl_805F9240[];                    // "hdrRed"
extern const char lbl_805F9248[];                    // "hdrBlue"
extern void* lbl_805FB528;
extern void* lbl_805FB52C;
extern u8* lbl_805FB8D8;
extern u32 lbl_805FB1EC;
extern u8 lbl_805FB1F0;
extern const char lbl_8036045C[];
}

typedef u8* (*GetFn)(void*);                  // object vtable +0xC: returns a sub-object
typedef void (*OverlayFn)(void*, s32, u32);   // overlay table (lbl_80348F38) +0x24: (target, id, float bits)

// Current value of a monster-builder slot list: the item at index +0x190 when it is within the
// count at +0x198, else -1.
static inline s32 SaveMonsterSlotValue(u8* slot) {
    s32 count = *(s32*)(slot + 0x198);
    if (count > 0) {
        s32 idx = *(s32*)(slot + 0x190);
        if (idx < count) return *(s32*)(slot + idx * 4);
    }
    return -1;
}

typedef s32 (*RecipeVFn)(void* obj, u32 hash);

// Shared body of the Recipes_Get*/Has* script callbacks: lbl_805FB248 is a stack of recipe-page
// objects (count/index at +0, objects from +4). If the "Recipes" page (hash 0x9C2C1CAF) is at
// index 1 and accepts the query, run onRecipes; else if the "parts" page (0x9E14654E) is at
// index 0, run onParts. Returns 0 when neither applies.
static inline bool RecipePageActive(u8* st, s32 idx, u32 hash) {
    if (st == NULL || *(s32*)st != idx) return false;
    void* obj = *(void**)(st + idx * 4 + 4);
    return (*(RecipeVFn*)(*(u8**)obj + 0x24))(obj, hash) != 0;
}

static inline s32 RecipePageQuery(void* obj, u32 hash) {
    if (obj == NULL) return 0;
    return (*(RecipeVFn*)(*(u8**)obj + 0x28))(obj, hash);
}

extern "C" {

// 0x800F9100
// Selects the challenge projector kind (stored at +0x1AC) from a name hash; 9 if unknown.
void SetupChallengeProjector(void* a0, u32 a1, u32 a2) {
    u8* p = (u8*)a0;
    if (p != NULL && (*(s32*)(*(u8**)(p + 0x4) + 0x1C) & 0x10000)) {
        fn_8006D248((s32)a2);
        switch (a1) {
        case 0xA41D84B9u: *(s32*)(p + 0x1AC) = 0; break;
        case 0xC4CE4FC3u: *(s32*)(p + 0x1AC) = 1; break;
        case 0x711A757Du: *(s32*)(p + 0x1AC) = 2; break;
        case 0x0144CE0Du: *(s32*)(p + 0x1AC) = 3; break;
        case 0x1EF3AA63u: *(s32*)(p + 0x1AC) = 4; break;
        case 0x01C00E16u: *(s32*)(p + 0x1AC) = 5; break;
        case 0x4B9FE6EBu: *(s32*)(p + 0x1AC) = 6; break;
        case 0xBAC3287Fu: *(s32*)(p + 0x1AC) = 7; break;
        case 0x5D664B82u: *(s32*)(p + 0x1AC) = 8; break;
        default:          *(s32*)(p + 0x1AC) = 9; break;
        }
    }
}

// 0x800D5FDC
// Depth-first search through a node graph (nodes have 4 edge slots at +4, edge flags at +0x24).
// Returns the slot index of the first edge on a path to a node whose id (+0) equals target, else -1.
// ctx holds a visited list: count at +0x400, entries from +4.
s32 fn_800D5FDC(u8* ctx, u8* node, u32 target, s32 flag) {
    if (node == NULL) return -1;
    if (*(u32*)node == target) return -1;
    s32 result = -1;
    u32 mask = 0;
    for (s32 i = 0; i < 4; i++) {
        if (*(s32*)(node + 4 + i * 4) != 0 && (*(s32*)(node + 0x24 + i * 4) == 0 || flag != 0))
            mask |= 1u << i;
    }
    u8* slot = node;
    for (s32 i = 0; i < 4; i++, slot += 4) {
        if (!(mask & (1u << i))) continue;
        u8* other = NULL;
        u8* edge = *(u8**)(slot + 4);
        if (edge != NULL) {
            other = *(u8**)(edge + 4);
            if (node == other) other = *(u8**)(edge + 8);
        }
        if (other == NULL) continue;
        if (*(u32*)other == target) return i;
        s32 count = *(s32*)(ctx + 0x400);
        s32 seen = 0;
        for (s32 k = 0; k < count; k++) {
            if (*(u8**)(ctx + 4 + k * 4) == other) { seen = 1; break; }
        }
        if (seen) continue;
        *(u8**)(ctx + 4 + count * 4) = other;
        *(s32*)(ctx + 0x400) = *(s32*)(ctx + 0x400) + 1;
        result = fn_800D5FDC(ctx, other, target, flag);
        if (result != -1) return i;
    }
    return result;
}

// 0x8016A64C
// Advances an animation/track time (+0x24) by dt, wrapping at the track length (+0x20 of the
// track at +0x14) and flagging a wrap at +0x1C; on a backward wrap reloads the per-key pointers.
void fn_8016A64C(u8* p, f32 dt) {
    u8* track = *(u8**)(p + 0x14);
    if (track == NULL) return;
    if (*(s32*)(p + 0x20) != 0) return;
    f32 old = *(f32*)(p + 0x24);
    f32 t = old + dt;
    *(f32*)(p + 0x24) = t;
    if (t < 0.0f) {
        *(s32*)(p + 0x1C) = 1;
        do {
            t = *(f32*)(p + 0x24) + *(f32*)(*(u8**)(p + 0x14) + 0x20);
            *(f32*)(p + 0x24) = t;
        } while (t < 0.0f);
    } else if (t >= *(f32*)(track + 0x20)) {
        *(s32*)(p + 0x1C) = 1;
        f32 len;
        do {
            u8* tr = *(u8**)(p + 0x14);
            len = *(f32*)(tr + 0x20);
            t = *(f32*)(p + 0x24) - len;
            *(f32*)(p + 0x24) = t;
            len = *(f32*)(tr + 0x20);
        } while (t >= len);
    }
    if (*(f32*)(p + 0x24) < old) {
        u8** out = (u8**)(p + 0x28);
        s32 off = 0;
        for (s32 i = 0; i < *(s32*)(p + 0x10); i++) {
            u8* tr = *(u8**)(p + 0x14);
            s32 rel = *(s32*)(tr + *(s32*)(tr + 0x24) + off);
            off += 8;
            *out = tr + rel;
            out++;
        }
    }
    fn_8016A43C(p);
    fn_8016A788(p);
}

// 0x8013FC1C
// After a multiplayer fight: save options or save data unless the game is quitting.
void PostMultiplayerFight(void) {
    if (IsGameFlowQuiting() != 0) return;
    BOOL c1 = fn_801F56FC(0x10) && fn_80142358();
    BOOL c2 = FALSE;
    BOOL any;
    if (c1) {
        any = TRUE;
    } else {
        c2 = (fn_801F56FC(4) || fn_801F56FC(8)) && fn_80142358();
        any = c2 || fn_80142468() == 1;
    }
    if (any && IsMultiPlayer() != 0) return;
    if ((fn_801F56FC(4) || fn_801F56FC(8)) && fn_80142358()) return;
    if (*(s32*)(lbl_805FBBE0 + 0x138) == 1) {
        SaveManager_SaveOptions();
        return;
    }
    SaveManager_SaveData();
}

// 0x80074990
// Pushes the experiment result UI screens for each flag that is set, then the final screen.
void PushExperimentUI(void) {
    static const s32 kFlags[] = { 1, 2, 0x4000, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 0x200,
                                  0x400, 0x800, 0x1000, 0x2000, 0x8000 };
    s32 first = *fn_800701A4();
    if (first != 0) UIScreen_Push((u32)first);
    for (u32 i = 0; i < sizeof(kFlags) / sizeof(kFlags[0]); i++) {
        if (fn_8006FA5C(fn_800701A4(), kFlags[i]) != 0) UIScreen_Push(fn_80070270(kFlags[i]));
    }
    UIScreen_Push(0xC62FBC4Eu);
}

// 0x801456E4
// Quaternion from a rotation matrix (3x4 row-major floats at m); result normalized.
void QuatFromMtx(Quat* out, f32* m) {
    f32 m0 = m[0], m4 = m[4], m8 = m[8];
    f32 tr = m8 + (m0 + m4);
    f32 w, x, y, z;
    if (tr > 0.0f) {
        x = m[5] - m[7];
        w = 1.0f + tr;
        y = m[6] - m[2];
        z = m[1] - m[3];
    } else if (m8 > m0 && m8 > m4) {
        w = m[1] - m[3];
        z = 1.0f + ((m8 - m0) - m4);
        x = m[6] + m[2];
        y = m[7] + m[5];
    } else if (m4 > m0) {
        w = m[6] - m[2];
        y = 1.0f + ((m4 - m8) - m0);
        z = m[5] + m[7];
        x = m[3] + m[1];
    } else {
        w = m[5] - m[7];
        x = 1.0f + ((m0 - m4) - m8);
        y = m[1] + m[3];
        z = m[2] + m[6];
    }
    out->x = x;
    out->y = y;
    out->z = z;
    out->w = w;
    f32 len2 = w * w + (z * z + (x * x + y * y));
    // asm: frsqrte plus two Newton steps; 1/sqrtf equals it to float precision
    f32 inv = (f32)(1.0 / sqrt((double)len2));
    out->x = x * inv;
    out->y = y * inv;
    out->z = z * inv;
    out->w = w * inv;
}

// 0x801453BC
// Writes the quaternion's rotated Y-axis-ish column: (x, y, z) = (2yx-2zw, 1-2x^2-2z^2, 2zy+2xw).
void QuatGetAxisXY(Quat* q, Vec3* out) {
    f32 x = q->x, y = q->y, z = q->z, w = q->w;
    f32 x2 = 2.0f * x, y2 = 2.0f * y, z2 = 2.0f * z;
    out->z = z2 * y + x2 * w;
    out->y = 1.0f - (x2 * x + z2 * z);
    out->x = y2 * x - z2 * w;
}

// 0x801454CC
// Writes the quaternion's rotated forward vector: (1-2y^2-2z^2, 2yx+2zw, 2zx-2yw).
void QuatGetForward(Vec3* out, Quat* q) {
    f32 x = q->x, y = q->y, z = q->z, w = q->w;
    f32 x2 = 2.0f * x, y2 = 2.0f * y, z2 = 2.0f * z;
    out->x = 1.0f - (y2 * y + z2 * z);
    out->z = z2 * x - y2 * w;
    out->y = y2 * x + z2 * w;
}

// 0x801151F4
ScriptValue Recipes_GetKeyIngredientImage(void) {
    u8* st = lbl_805FB248;
    if (RecipePageActive(st, 1, 0x9C2C1CAFu)) {
        if (st != NULL) { s32 r = RecipePageQuery(*(void**)(st + 8), 0x9C2C1CAFu); if (r != 0) return (ScriptValue)fn_80120444(r); }
        return 0;
    }
    st = lbl_805FB248;
    if (RecipePageActive(st, 0, 0x9E14654Eu)) {
        s32 r = RecipePageQuery(st ? *(void**)(st + 4) : NULL, 0x9E14654Eu);
        if (r != 0) return (ScriptValue)fn_8011BE84((u8*)(intptr_t)r);
        return 0;
    }
    return 0;
}

// 0x80115370
ScriptValue Recipes_GetKeyIngredientName(void) {
    u8* st = lbl_805FB248;
    if (RecipePageActive(st, 1, 0x9C2C1CAFu)) {
        if (st != NULL) { s32 r = RecipePageQuery(*(void**)(st + 8), 0x9C2C1CAFu); if (r != 0) return (ScriptValue)fn_80120474(r); }
        return 0;
    }
    st = lbl_805FB248;
    if (RecipePageActive(st, 0, 0x9E14654Eu)) {
        s32 r = RecipePageQuery(st ? *(void**)(st + 4) : NULL, 0x9E14654Eu);
        if (r != 0) return (ScriptValue)fn_8011BEB0((u8*)(intptr_t)r);
        return 0;
    }
    return 0;
}

// 0x801154EC
ScriptValue Recipes_GetSecondaryIngredientImage(void) {
    u8* st = lbl_805FB248;
    if (RecipePageActive(st, 1, 0x9C2C1CAFu)) {
        if (st != NULL) { s32 r = RecipePageQuery(*(void**)(st + 8), 0x9C2C1CAFu); if (r != 0) return (ScriptValue)fn_801204A4(r); }
        return 0;
    }
    st = lbl_805FB248;
    if (RecipePageActive(st, 0, 0x9E14654Eu)) {
        s32 r = RecipePageQuery(st ? *(void**)(st + 4) : NULL, 0x9E14654Eu);
        if (r != 0) return (ScriptValue)fn_8011BEDC((u8*)(intptr_t)r);
        return 0;
    }
    return 0;
}

// 0x80115668
ScriptValue Recipes_GetSecondaryIngredientName(void) {
    u8* st = lbl_805FB248;
    if (RecipePageActive(st, 1, 0x9C2C1CAFu)) {
        if (st != NULL) { s32 r = RecipePageQuery(*(void**)(st + 8), 0x9C2C1CAFu); if (r != 0) return (ScriptValue)fn_801204D4(r); }
        return 0;
    }
    st = lbl_805FB248;
    if (RecipePageActive(st, 0, 0x9E14654Eu)) {
        s32 r = RecipePageQuery(st ? *(void**)(st + 4) : NULL, 0x9E14654Eu);
        if (r != 0) return (ScriptValue)fn_8011BF08((u8*)(intptr_t)r);
        return 0;
    }
    return 0;
}

// 0x80115078
s32 Recipes_HasSecondaryIngredient(void) {
    u8* st = lbl_805FB248;
    if (RecipePageActive(st, 1, 0x9C2C1CAFu)) {
        if (st != NULL) { s32 r = RecipePageQuery(*(void**)(st + 8), 0x9C2C1CAFu); if (r != 0) return fn_801203F0(r); }
        return 0;
    }
    st = lbl_805FB248;
    if (RecipePageActive(st, 0, 0x9E14654Eu)) {
        s32 r = RecipePageQuery(st ? *(void**)(st + 4) : NULL, 0x9E14654Eu);
        if (r != 0) return fn_8011BE4C((u8*)(intptr_t)r);
        return 0;
    }
    return 0;
}
// 0x800ED59C
// Removes the first lightning effect (matching a1 in +0x4C) from the zone's effect chain.
void RemoveLightning(void* a0, u32 a1) {
    u16 idx = 0;
    if (a0 != NULL) idx = *(u16*)(*(u8**)((u8*)a0 + 0x4) + 0x32);
    u8* tbl = *(u8**)(*(u8**)gWorldState + 0x1C8);
    u16 id = *(u16*)(tbl + idx * 0xC + 0x2);
    u32 hash = lbl_805FB1EC;
    u8* node = fn_801E7424(lbl_805FB8D8, &id);
    while (node != NULL) {
        if ((s8)lbl_805FB1F0 == 0) {
            hash = StringHash(lbl_8036045C);  // "Lightning"
            lbl_805FB1EC = hash;
            lbl_805FB1F0 = 1;
        }
        if (*(u32*)(node + 0x64) == hash && a1 == *(u32*)(node + 0x4C)) break;
        u16 next = *(u16*)(node + 0x7A);
        u8* base = lbl_805FB8D8;
        id = next;
        if (fn_801E7460(base, &id)) node = base + ((next & 0xFF) << 7);
        else node = NULL;
    }
    if (node != NULL) {
        u16 h = *(u16*)(node + 0x7C);
        fn_801E9678(*(void**)(node + 0x74), &h);
    }
}

// 0x800E8278
// Resets an entity for respawn: clears state flags, posts event 0x64, clears bit 2 of its
// controller flags and bumps the respawn counter (and the world counter for flagged entities).
void RespawnEntity(void* a0) {
    u8* e = (u8*)a0;
    ScopedLock lock;
    if (*(s32*)(*(u8**)(e + 0x4) + 0x20) & 3) {
        ScopedLockCtor(&lock);
        (*(void (**)(void*, s32))(*(u8**)e + 0x18))(e, 1);
        ScopedLockDtor(&lock, -1);
    }
    if (!(*(s32*)(*(u8**)(e + 0x4) + 0x20) & 0x04000000)) {
        u8* ev = (u8*)EventPost(0x64, e);
        if (ev != NULL) *(s32*)(ev + 0x4) = 0;
    }
    if ((*(u8* (**)(void*))(*(u8**)e + 0xC))(e) != NULL) {
        u8* c = (*(u8* (**)(void*))(*(u8**)e + 0xC))(e);
        *(s32*)(c + 0x8) &= ~2;
    }
    u8* st = *(u8**)(e + 0x4);
    *(s32*)(st + 0x20) &= 0xFFFDF8FC;
    st = *(u8**)(e + 0x4);
    *(s32*)(st + 0x20) |= 0x20000;
    *(u8*)(*(u8**)(e + 0x4) + 0x35) = 0;
    st = *(u8**)(e + 0x4);
    s32 n = *(s32*)(st + 0x28);
    if (n > 0x3E8) {
        *(s32*)(st + 0x28) = n + 1;
        if (*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x10) *(s32*)(gWorldState + 0xE4) += 1;
    }
}

// 0x800F84A8
// Sets an entity's orientation from euler angles scaled by 2/pi and flags it for update.
void RotateEntityAroundAxis(u32 a0, f32 a1, f32 a2, f32 a3) {
    Quat q;
    Vec3 euler;
    u8* ent = (u8*)CharacterFind((s32)a0);
    euler.x = a1 * 0.63661975f;
    euler.y = a2 * 0.63661975f;
    euler.z = a3 * 0.63661975f;
    EulerToQuat(&q, &euler);
    u8* st = *(u8**)(ent + 0x4);
    *(f32*)(st + 0xC) = q.x;
    *(f32*)(st + 0x10) = q.y;
    *(f32*)(st + 0x14) = q.z;
    *(f32*)(st + 0x18) = q.w;
    *(s32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x8000;
    *(s32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x10000000;
    *(s32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x4000;
    *(s32*)(*(u8**)(ent + 0x4) + 0x20) |= 0x10000000;
}
// 0x80124130
// TRUE if one of the first lbl_805FB2C8 saved profiles has the given name (profile name at
// +0x268 of each 0x988-byte slot). The save buffer is 0x31C0 bytes with the four slots at +0xB98.
u32 SaveManager_DoesProfileNameExist(BOOL a0) {
    alignas(32) u8 save[0x31C0];
    if (lbl_805FB2D5 != 0) return 0;
    u8* slots = save + 0xB98;
    fn_8032F1E8(slots, (void (*)(void))fn_8022ED60, (void (*)(void))fn_8022ED98, 0x988, 4);
    SaveGameRead("save", save, 0x31C0);
    s32 count = lbl_805FB2C8;
    u8* slot = slots;
    for (s32 i = 0; i < 4; i++, slot += 0x988) {
        if (i != count && strcmp((const char*)(intptr_t)a0, (const char*)(slot + 0x268)) == 0) {
            __destroy_arr(slots, (void (*)(void*, s32))fn_8022ED98, 0x988, 4);
            return 1;
        }
    }
    __destroy_arr(slots, (void (*)(void*, s32))fn_8022ED98, 0x988, 4);
    return 0;
}

// 0x801230DC
// Loads the save file into the live state: options (volumes, brightness, rumble), profile
// records and the per-slot name/stat table lbl_80556E80; defaults when there is no valid save.
void SaveManager_Init(void) {
    alignas(32) u8 save[0x31C0];
    lbl_805FB2D5 = 0;
    lbl_805FB2C8 = 0;
    SaveResetBanner();
    if (SaveFileExists(lbl_805F8FD4, lbl_805F8FD4) != 0 && SaveManager_IsSaveDataCorrupted() == 0) {
        u8* slots = save + 0xB98;
        fn_8032F1E8(slots, (void (*)(void))fn_8022ED60, (void (*)(void))fn_8022ED98, 0x988, 4);
        SaveGameRead("save", save, 0x31C0);
        lbl_805FB2CC = *(s16*)(save + 0xB80);
        SetOptionsMusicVolume(*(u16*)(save + 0xB84));
        SetOptionsVoiceVolume(*(u16*)(save + 0xB86));
        SetOptionsSfxVolume(*(u16*)(save + 0xB82));
        SetOptionsBrightness(*(u16*)(save + 0xB88));
        SetOptionsRumble(*(u8*)(save + 0xB8A));
        fn_800FB438(*(s32*)(save + 0x0));
        fn_800FB450(*(s32*)(save + 0x4));
        *(s32*)(lbl_805FBBE0 + 0x920) = *(s32*)(save + 0xB8C);
        *(s32*)(lbl_805FBBE0 + 0x924) = *(s32*)(save + 0xB90);
        for (s32 k = 0; k < 16; k++) *(s32*)(lbl_80556E80 + k * 4) = *(s32*)(save + 0x20 + k * 4);
        u8* src = save;
        u8* dst = lbl_80556E80;
        for (s32 i = 0; i < 0x40; i++) {
            *(s32*)(dst + 0x40) = *(s32*)(src + 0x60);
            *(s32*)(dst + 0x44) = *(s32*)(src + 0x64);
            *(s32*)(dst + 0x48) = *(s32*)(src + 0x68);
            strcpy((char*)(lbl_80556E80 + 0x340 + i * 0x20), (const char*)(save + 0x360 + i * 0x20));
            *(s32*)(lbl_80556E80 + 0x354 + i * 0x20) = *(s32*)(save + 0x374 + i * 0x20);
            *(s32*)(lbl_80556E80 + 0x350 + i * 0x20) = *(s32*)(save + 0x370 + i * 0x20);
            *(s32*)(lbl_80556E80 + 0x35C + i * 0x20) = *(s32*)(save + 0x37C + i * 0x20);
            *(s32*)(lbl_80556E80 + 0x358 + i * 0x20) = *(s32*)(save + 0x378 + i * 0x20);
            src += 0xC;
            dst += 0xC;
        }
        memcpy(lbl_80556E80 + 0xB40, save + 0xB60, 0x10);
        __destroy_arr(slots, (void (*)(void*, s32))fn_8022ED98, 0x988, 4);
    } else {
        lbl_805FB2CC = -1;
        SetOptionsMusicVolume(4);
        SetOptionsSfxVolume(5);
        SetOptionsVoiceVolume(7);
        SetOptionsBrightness(5);
        SetOptionsRumble(1);
        fn_800FB438(0);
        fn_800FB450(0);
        memset(lbl_80556E80, 0, 0xB60);
    }
}

// 0x80123550
// Validates the save file. Returns 1 (and sets the save-error flag) when reading it fails;
// otherwise checks the save media and the CRC of the data block, resetting the save media and
// setting up the banner when invalid. Returns 0 if the file exists and was not rejected.
u32 SaveManager_IsSaveDataCorrupted(void) {
    alignas(32) u8 save[0x31C0];
    u8 handle[0xA0];
    if (SaveFileExists(lbl_805F8FD4, lbl_805F8FD4) == 0) return 0;
    u8* slots = save + 0xB98;
    s32 bad = 0;
    fn_8032F1E8(slots, (void (*)(void))fn_8022ED60, (void (*)(void))fn_8022ED98, 0x988, 4);
    s32 r = SaveGameRead("save", save, 0x31C0);
    if (r == -4 || r == -1 || r == -0xF || r == -5 || r == -8) {
        lbl_805FB2D5 = 1;
        __destroy_arr(slots, (void (*)(void*, s32))fn_8022ED98, 0x988, 4);
        return 1;
    }
    if (fn_80206DA4() == 0) bad = 1;
    if (fn_80266BA4(lbl_805F8FD0, handle, 1) != 0) bad = 1;
    u8* mem = (u8*)MemAllocSmall(0xF0C0);
    u8* aligned = (u8*)(((uintptr_t)mem + 0x1F) & ~(uintptr_t)0x1F);
    if (mem == NULL) bad = 1;
    if (bad == 0) {
        r = fn_80265F80(handle, aligned, 0xF0A0);
        fn_80266E24(handle);
        if (r == -4 || r == -1 || r == -0xF || r == -5 || r == -8) bad = 1;
    }
    if (bad == 0) {
        u32 crc = fn_80143974(0, aligned, *(u32*)(save + 0xB90));
        if (crc != *(u32*)(save + 0xB8C) || (u32)r != *(u32*)(save + 0xB90)) bad = 1;
    }
    if (mem != NULL) MemFreeSmall(mem);
    if (bad != 0) {
        fn_80265D78((void*)lbl_805F8FD0);
        SaveSetupBanner();
    }
    __destroy_arr(slots, (void (*)(void*, s32))fn_8022ED98, 0x988, 4);
    return 0;
}

// 0x80123B78
// Writes the current profile slot (lbl_805FB2C8) into the save file (loading the existing file
// first if there is one); writes a second time with the previous slot when the save block has no
// completed flag (+0x924).
void SaveManager_SaveData(void) {
    alignas(32) u8 save[0x31C0];
    if (lbl_805FB2D5 != 0) return;
    u8* slots = save + 0xB98;
    fn_80142870(0);
    fn_8032F1E8(slots, (void (*)(void))fn_8022ED60, (void (*)(void))fn_8022ED98, 0x988, 4);
    if (SaveFileExists(lbl_805F8FD4, lbl_805F8FD4) != 0) SaveGameRead("save", save, 0x31C0);
    s32 complete = *(s32*)(lbl_805FBBE0 + 0x924);
    fn_80123954(lbl_805FB2C8, save);
    fn_8022F50C(slots + lbl_805FB2C8 * 0x988);
    SaveGameWrite("save", save, 0x31C0);
    if (complete == 0) {
        fn_80123954(lbl_805FB2CC, save);
        SaveGameWrite("save", save, 0x31C0);
    }
    lbl_805FB2CC = lbl_805FB2C8;
    fn_80142870(1);
    __destroy_arr(slots, (void (*)(void*, s32))fn_8022ED98, 0x988, 4);
}
// 0x8001E03C
// Commits the monster being built in the lab UI (lbl_805FAF78) into the save: a new monster
// when the selected slot (+0xAD8) is negative, else updates the existing one, then sets its name.
void SaveMonster(void) {
    u8* ui = lbl_805FAF78;
    u8* save;
    if (*(s32*)ui == 0) return;
    s32 sel = *(s32*)(ui + 0xAD8);
    if (sel < 0) {
        u8* mon = LabAllocMonster(lbl_805FBBE0 + 0xA30);
        for (s32 i = 0; i < 5; i++) {
            u8* cur = lbl_805FAF78;
            (*(void (**)(void*, s32))(*(u8**)(cur + 0x940) + 0x18))(cur + 0x940, i);
            u8* slot = lbl_805FAF78 + i * 0x1A4 + 8;
            s32 v = SaveMonsterSlotValue(slot);
            if (v >= 0) {
                s32 v2 = SaveMonsterSlotValue(slot);
                (*(void (**)(void*, s32, s32))(*(u8**)mon + 0x14))(mon, i, *(s32*)(lbl_805FBBE0 + v2 * 4 + 0xA58));
            }
        }
        MonsterSetName(mon, (const char*)(lbl_805FAF78 + 0xAE8));
        fn_80226C3C(lbl_805FBBE0, 1);
        *(u8**)(lbl_805FBBE0 + 0x114) = mon;
        return;
    }
    save = lbl_805FBBE0;
    u8* mon = *(u8**)(save + sel * 4 + 0xA30);
    if (mon == NULL) return;
    for (s32 i = 0; i < 5; i++) (*(void (**)(void*, s32))(*(u8**)mon + 0x18))(mon, i);
    for (s32 i = 0; i < 5; i++) {
        s32 part = 0;
        u8* ui2 = lbl_805FAF78;
        u8* slot = ui2 + i * 0x1A4 + 8;
        if (SaveMonsterSlotValue(slot) >= 0 && *(s32*)(ui2 + i * 4 + 0x944) != 0) {
            s32 v2 = SaveMonsterSlotValue(slot);
            part = *(s32*)(lbl_805FBBE0 + v2 * 4 + 0xA58);
        }
        if (part != 0) {
            (*(void (**)(void*, s32))(*(u8**)(ui2 + 0x940) + 0x18))(ui2 + 0x940, i);
            (*(void (**)(void*, s32, s32))(*(u8**)mon + 0x14))(mon, i, part);
        }
    }
    MonsterSetName(mon, (const char*)(lbl_805FAF78 + 0xAE8));
    *(u8**)(lbl_805FBBE0 + 0x114) = mon;
}

// 0x801DC658
// Finalizes a script signature record: +0xC = sum of per-type argument sizes (-2 on type 0xA),
// +8 = arg count clamped to 8, +0x10 = per-arg flag bits (0x10000<<i for type 0x10, 0x100<<i for
// 0x17, 1<<i when fn_801DC3AC, 0x01000000<<i when fn_801DC4D4), types are stored from +0x14.
void ScriptSigFinalize(u8* a) {
    *(s32*)(a + 0xC) = 0;
    u8* p = a;
    u32 i = 0;
    while (i < *(u32*)(a + 0x8) && *(s32*)(a + 0xC) != -2) {
        s32 t = *(s32*)(p + 0x14);
        if (t == 0xA) {
            *(s32*)(a + 0xC) = -2;
        } else {
            *(s32*)(a + 0xC) = *(s32*)(a + 0xC) + fn_801DC430(t);
        }
        p += 4;
        i++;
    }
    if (*(u32*)(a + 0x8) > 8) *(u32*)(a + 0x8) = 8;
    p = a;
    *(s32*)(a + 0x10) = 0;
    for (u32 k = 0; k < *(u32*)(a + 0x8); k++) {
        s32 t = *(s32*)(p + 0x14);
        if (t == 0x10) {
            *(s32*)(a + 0x10) |= 0x10000 << k;
        } else if (t == 0x17) {
            *(s32*)(a + 0x10) |= 0x100 << k;
        } else if (fn_801DC3AC(t) != 0) {
            *(s32*)(a + 0x10) |= 1 << k;
        }
        if (fn_801DC4D4(t) != 0) *(s32*)(a + 0x10) |= 0x01000000 << k;
        p += 4;
    }
}

// 0x800F1B98
// Moves an enemy to the position of the path junction with the given id (only in phase 2).
void SetEnemyToJunction(void* a0, u32 a1) {
    u8* e = (u8*)a0;
    u8 ref[0x404];
    if (e == NULL || WorldGetPhase(gGame + 0xD5F0) != 2) return;
    u8* rec = ObjDbLookup(gObjDb, (s32)a1);
    memcpy(ref, rec, 0x404);
    Vec3* pos = PathGetPosition((PathFollower*)ref);
    f32 x = pos->x, y = pos->y, z = pos->z;
    u8* st = *(u8**)(e + 0x4);
    *(f32*)(st + 0x0) = x;
    *(f32*)(st + 0x4) = y;
    *(f32*)(st + 0x8) = z;
    *(s32*)(*(u8**)(e + 0x4) + 0x20) |= 0x8000;
    *(s32*)(*(u8**)(e + 0x4) + 0x20) |= 0x10000000;
    *(s32*)(*(u8**)(e + 0x4) + 0x20) |= 0x4000;
    *(s32*)(*(u8**)(e + 0x4) + 0x20) |= 0x10000000;
    fn_800D3C60(gObjDb, *(void**)(e + 0xA00), y, x);
    fn_80053550(e, 0);
    fn_80053418(e);
    ObjRefDtor(ref, -1);
}

// 0x800F590C
// Sets the RGB overlay colour (ids 0x135..0x137) on every renderable of the entity.
void SetEntityColor0(void* a0, f32 a1, f32 a2, f32 a3) {
    u8* e = (u8*)a0;
    for (s32 i = 0; i < *(s32*)((u8*)fn_801DA758((*(GetFn*)(*(u8**)e + 0xC))(e)) + 0x6C); i++) {
        u8* tbl = *(u8**)((u8*)fn_801DA758((*(GetFn*)(*(u8**)e + 0xC))(e)) + 0x10);
        u8* r = *(u8**)(tbl + i * 8 + 0x70);
        if (r != NULL) {
            (*(OverlayFn*)(lbl_80348F38 + *(u8*)(r + 0xC) * 4 + 0x24))(*(void**)(r + 0x8), 0x135, *(u32*)&a1);
            (*(OverlayFn*)(lbl_80348F38 + *(u8*)(r + 0xC) * 4 + 0x24))(*(void**)(r + 0x8), 0x136, *(u32*)&a2);
            (*(OverlayFn*)(lbl_80348F38 + *(u8*)(r + 0xC) * 4 + 0x24))(*(void**)(r + 0x8), 0x137, *(u32*)&a3);
        }
    }
}
// Show/hide an entity: bit 2 of the sub-object's flags (+8) and of the entity state flags (+0x20).
static void EntitySetShown(u8* e, s32 shown) {
    GetFn getSub = *(GetFn*)(*(u8**)e + 0xC);
    if (shown) {
        if (getSub(e) != NULL) {
            u8* sub = (*(GetFn*)(*(u8**)e + 0xC))(e);
            *(s32*)(sub + 0x8) |= 2;
        }
        *(s32*)(*(u8**)(e + 0x4) + 0x20) |= 2;
    } else {
        if (getSub(e) != NULL) {
            u8* sub = (*(GetFn*)(*(u8**)e + 0xC))(e);
            *(s32*)(sub + 0x8) &= 0xFFFFFFFD;
        }
        *(s32*)(*(u8**)(e + 0x4) + 0x20) &= 0xFFFFFFFD;
    }
}

// 0x800E8464
// Shows or hides an entity; entities with state flag 0x2000 also toggle byte +0xC87.
void SetEntityVisibility(void* a0, s32 a1) {
    u8* e = (u8*)a0;
    if (e != NULL) EntitySetShown(e, a1 != 0);
    if (*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x2000) {
        if (a1 != 0) {
            *(u8*)(e + 0xC87) = 0;
            EntitySetShown(e, 1);
        } else {
            *(u8*)(e + 0xC87) = 1;
            EntitySetShown(e, 0);
        }
    }
}

// Maps a brightness level (0..10) onto the colour-curve entries at lbl_8040D500: entry +4 gets a
// gamma in [1.0, 1.4] and entry +8 a bias in [-0.15, 0.15] taken from the world block (+0x1A84/+0x1A88),
// interpolated around the middle level.
static void ApplyBrightness(u32 level) {
    s32 lv = 10;
    if (level < 10) lv = (s32)level;
    f32 t = (f32)lv / 10.0f;
    u8* world = *(u8**)gWorldState;
    f32 g = *(f32*)(world + 0x1A84);
    f32 gamma = 1.0f;
    if (!(g < 1.0f)) {
        gamma = 1.4f;
        if (!(g > 1.4f)) gamma = g;
    }
    f32 b = *(f32*)(world + 0x1A88);
    f32 bias = -0.15f;
    if (!(b < -0.15f)) {
        bias = 0.15f;
        if (!(b > 0.15f)) bias = b;
    }
    if (t < 0.5f) {
        f32 u = t / 0.5f;
        f32 v = 1.4f + u * (gamma - 1.4f);
        *(f32*)(fn_8015EC9C(lbl_8040D500, 1.4f, u) + 4) = v;
        u = t / 0.5f;
        v = -0.15f + u * (bias - -0.15f);
        *(f32*)(fn_8015EC9C(lbl_8040D500, -0.15f, u) + 8) = v;
    } else {
        f32 u = (t - 0.5f) / 0.5f;
        f32 v = gamma + u * (1.0f - gamma);
        *(f32*)(fn_8015EC9C(lbl_8040D500, u, 0.5f) + 4) = v;
        u = (t - 0.5f) / 0.5f;
        v = bias + u * (0.15f - bias);
        *(f32*)(fn_8015EC9C(lbl_8040D500, u, 0.5f) + 8) = v;
    }
}

// 0x800FB0B4
// Applies the brightness option level and stores it (u16) in the save block at +0x91A.
void SetOptionsBrightness(BOOL a0) {
    ApplyBrightness((u32)a0);
    *(u16*)(lbl_805FBBE0 + 0x91A) = (u16)a0;
}

// 0x800FAF30
// Applies the brightness level without saving it.
void SetOptionsBrightnessTmp(BOOL a0) {
    ApplyBrightness((u32)a0);
}

// 0x800F76DC
// Shows or hides the player avatar (entity gGame+0xD614) and its five attachments (+0xC50..).
void SetPlayerAvatarRenderFlag(u32 a0) {
    u8* e = *(u8**)(gGame + 0xD614);
    if (*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x2000) {
        u8* p = e;
        for (s32 i = 0; i < 5; i++, p += 4) {
            if (*(s32*)(p + 0xC50) != 0) fn_8004EDD0(*(u8**)(p + 0xC50), a0 != 0, 0);
        }
    }
    EntitySetShown(e, a0 != 0);
}

// 0x800F5AF0
// For each of the five field-repair slots passes health/max-health to fn_8002D508 (0 when max is 0).
void SetupFieldRepairHealthPoints(void) {
    u8* e = *(u8**)(gGame + 0xD614);
    if (e != NULL && (*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x2000)) {
        for (s32 i = 0; i < 5; i++) {
            f32 cur = (f32)fn_8004E2A4(e, i);
            f32 max = (f32)fn_8004E2C4(e, i);
            u32 extra = fn_8004E2E4(e, i);
            if (max != 0.0f) fn_8002D508(i, cur / max, extra);
            else fn_8002D508(i, 0.0f, extra);
        }
    }
}
// Clears the 0x80-byte overlay-tween descriptor and sets the common defaults: update callback at
// +0x6C, 0xFFFF handles at +0x78/+0x7A/+0x7C.
static void ShaderTweenDescInit(u8* d, void (*cb)(void)) {
    *(u16*)(d + 0x78) = 0xFFFF;
    *(u16*)(d + 0x7A) = 0xFFFF;
    *(u16*)(d + 0x7C) = 0xFFFF;
    memset(d, 0, 0x64);
    *(u32*)(d + 0x64) = 0;
    *(s32*)(d + 0x68) = 0;
    *(f32*)(d + 0x70) = 0.0f;
    *(u16*)(d + 0x78) = 0xFFFF;
    *(u16*)(d + 0x7A) = 0xFFFF;
    *(u16*)(d + 0x7C) = 0xFFFF;
    *(s32*)(d + 0x74) = 0;
    *(u8*)(d + 0x7E) = 0;
    *(u8*)(d + 0x7F) = 0;
    *(void (**)(void))(d + 0x6C) = cb;
}

// 0x801F252C
// Starts a fade of the overlay alpha on shader object a1 in the zone of a0 (zone id at a0+1):
// target alpha a2, duration a3 (negative treated as 0; rate 1/a3 when positive).
void ShaderFadeToOverlayAlpha(u32 a0, u32 a1, f32 a2, f32 a3) {
    u8* obj = (u8*)(uintptr_t)a1;
    u8 desc[0x80];
    u8 out[8];
    f32 target = a2;
    if (obj == NULL) return;
    s32 zone = *(u8*)((u8*)(uintptr_t)a0 + 1);
    if (ZoneIsLoaded(gWorldState, zone) == 0) return;
    u8* entry = *(u8**)(*(u8**)gWorldState + 0x1C8) + zone * 0xC;
    ShaderTweenDescInit(desc, (void (*)(void))fn_801F2018);
    *(u32*)(desc + 0x0) = a1;
    *(s32*)(desc + 0x4) = 0;
    *(f32*)(desc + 0x8) = target;
    f32 dur = 0.0f;
    if (0.0f <= a3) dur = a3;
    *(f32*)(desc + 0xC) = dur;
    f32 rate = 0.0f;
    if (dur > 0.0f) rate = 1.0f / a3;
    *(f32*)(desc + 0x10) = rate;
    *(u32*)(desc + 0x64) = StringHash((const char*)(obj + 0x80));
    fn_801E8D74(out, entry, NULL, desc);
}

// 0x801DB518
// Sets overlay alpha 1 - (distance from the shader object to the entity) / range (id 0x10 on the
// overlay target a2). Range is the object's own radius (fn_801EDD00), else a3, else 1.0.
void ShaderSetOverlayAlphaFromDistance(void* a0, u32 a1, u32 a2, f32 a3) {
    u8* ent = (u8*)a0;
    u8* obj = (u8*)(uintptr_t)a1;
    u8* tgt = (u8*)(uintptr_t)a2;
    if (tgt == NULL || ent == NULL || *(u16*)(obj + 0x2) == 0xFFFF) return;
    f32 range = fn_801EDD00(obj);
    if (range <= 0.0f) {
        if (a3 > 0.0f) range = a3;
        else range = 1.0f;
    }
    u32 key = *(u32*)obj;
    f32 pos[3];
    fn_801ED788(pos, (u8*)&key);
    u8* st = *(u8**)(ent + 0x4);
    f32 dx = *(f32*)(st + 0x0) - pos[0];
    f32 dy = *(f32*)(st + 0x4) - pos[1];
    f32 dz = *(f32*)(st + 0x8) - pos[2];
    f32 len2 = dz * dz + (dx * dx + dy * dy);
    f32 dist = 0.0f;
    if (!(len2 <= 0.0f)) {
        // asm: frsqrte plus two Newton steps times len2; sqrt equals it to float precision
        dist = (f32)sqrt((double)len2);
    }
    f32 rem = range - dist;
    f32 alpha;
    if (rem <= 0.0f) alpha = 0.0f;
    else if (rem > a3) alpha = 1.0f;
    else alpha = rem / a3;
    (*(OverlayFn*)(lbl_80348F38 + *(u8*)(tgt + 0xC) * 4 + 0x24))(*(void**)(tgt + 0x8), 0x10, *(u32*)&alpha);
}

// 0x801F2288
// Starts a bouncing overlay-alpha tween on shader object a1 in the current zone: period from a2
// (at least 0.01), alpha range between a3 and a4 (each clamped to 0..1).
void ShaderStartBouncingOverlayAlpha(u32 a0, u32 a1, f32 a2, f32 a3, f32 a4) {
    u8* obj = (u8*)(uintptr_t)a1;
    u8 desc[0x80];
    u8 out[8];
    if (obj == NULL) return;
    s32 zone = fn_801EDF58((u8*)(uintptr_t)a0);
    if (ZoneIsLoaded(gWorldState, zone) == 0) return;
    u8* entry = *(u8**)(*(u8**)gWorldState + 0x1C8) + zone * 0xC;
    if (a2 <= 0.01f) a2 = 0.01f;
    f32 inv = 1.0f / a2;
    ShaderTweenDescInit(desc, (void (*)(void))fn_801F1D64);
    *(f32*)(desc + 0x8) = 2.0f * inv;
    if (a3 >= 1.0f) a3 = 1.0f;
    f32 lo = 0.0f;
    if (0.0f <= a3) lo = a3;
    if (a4 >= 1.0f) a4 = 1.0f;
    f32 hi = 0.0f;
    if (0.0f <= a4) hi = a4;
    *(u32*)(desc + 0x0) = a1;
    *(s32*)(desc + 0x4) = 0;
    f32 mn = (lo < hi) ? lo : hi;
    *(f32*)(desc + 0xC) = mn;
    f32 mx = lo;
    if (lo <= hi) mx = hi;
    *(f32*)(desc + 0x10) = mx;
    *(u8*)(desc + 0x18) = 1;
    *(f32*)(desc + 0x14) = mx - mn;
    *(u32*)(desc + 0x64) = StringHash((const char*)(obj + 0x80));
    fn_801E8D74(out, entry, NULL, desc);
}
// 0x800EF9E4
// Opens the instruction screen: title/body text ids a0/a1, page-style element chosen by a2
// (0 and unknown -> 0xBA036DE3, 1 -> 0x0C9524AC, 2 -> 0xFD805850); a3 != 0 sends event 0x4F9213C1
// to every live character that qualifies.
void ShowInstructionScreen(u32 a0, u32 a1, BOOL a2, u32 a3) {
    if (lbl_805FB0E0 == 0) ScreenSetsActivate(gScreenSets, 0x7A1060A7u);
    UIScreen_Push(0x0C4FC83Bu);
    UIText_SetString_TextID(0xA0139595u, a0);
    UIText_SetString_TextID(0x0281E9F9u, a1);
    UIText_SetString(0x121100C6u, (BOOL)(intptr_t)lbl_805F8DA0);  // "1 / 1"
    switch (a2) {
    case 0: UIElement_SetVisibleFlag(0xBA036DE3u, 1); break;
    case 1: UIElement_SetVisibleFlag(0x0C9524ACu, 1); break;
    case 2: UIElement_SetVisibleFlag(0xFD805850u, 1); break;
    default: UIElement_SetVisibleFlag(0xBA036DE3u, 1); break;
    }
    if (a3 != 0) {
        u8** tbl = (u8**)(gGame + 0x140);
        for (s32 i = 0; i < 0x20; i++) {
            u8* c = tbl[i];
            if (c != NULL && (*(s32 (**)(void*))(*(u8**)c + 0x4C))(c) == 0) {
                c = tbl[i];
                if ((*(s32*)(*(u8**)(c + 0x4) + 0x1C) & 0x10) && *(u8*)(c + 0x895) == 0)
                    EventSend(gEventMgr, c, NULL, 0x4F9213C1u, 0, 0, 0, 0);
            }
        }
    }
}

// 0x8004859C
// Reward-screen step: pops the current reward item's element (when the active screen set is the
// reward screen) and sends the "next" event for the following slot, or the finish event when done.
void ShowNextRewardItem(void) {
    s32 start = -1;
    s32 end = *(s32*)((u8*)gLab + 0x8);
    if (*(s32*)((u8*)gLab + 0x3C) != 0) {
        start = 0;
        end += 1;
    }
    if (lbl_805FB0B8 > start && lbl_805FB0B8 < end) {
        u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
        if (set == NULL) return;
        if (*(u32*)set != 0x1B66AC94u) return;
        u8* el = (u8*)ScreenSetFindElement(set, lbl_80358B40[lbl_805FB0B8]);
        if (el != NULL) (*(void (**)(void*, s32))(*(u8**)(el + 0x138) + 0x2C))(el, 2);
    }
    s32 cur = lbl_805FB0B8;
    if (cur < end) {
        if (cur >= 4) return;
        UIEvent_SendToScreen(lbl_80358B30[cur], 0xCFBD5B9Eu);
        lbl_805FB0B8 = cur + 1;
    } else {
        UIEvent_SendToScreen(0x629C1D26u, 0xCFBD5B9Eu);
    }
}

// 0x8013F71C
// Pushes a screen (if the current screen set has it) and resets its widgets for a few known screens.
void ShowScreen(u32 a0) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    void* widget = ScreenSetFindWidget(set, a0);
    if (widget == NULL) return;
    UIScreen_Push(a0);
    lbl_805FB41C = 0;
    switch (a0) {
    case 0x7D12C9EEu: {
        void* c = UIWidgetFindChild(widget, 0x6C2E9B73u);
        if (c != NULL) UISetText(c, lbl_805F90D4, 1);
        break;
    }
    case 0x9631294Eu:
        UIElement_SetVisibleFlag(0x8FB60C54u, 0);
        UIElement_SetVisibleFlag(0x6530D136u, 0);
        break;
    case 0xFEE5B157u: {
        UIElement_SetVisibleFlag(0x60B15D91u, 0);
        void* c = UIWidgetFindChild(widget, 0x3E22E495u);  // "Health"
        if (c != NULL) UISetProgress(c, 0.0f);
        c = UIWidgetFindChild(widget, 0x67E9BE17u);  // "energy"
        if (c != NULL) UISetProgress(c, 0.0f);
        c = UIWidgetFindChild(widget, 0xB738EABDu);  // "attack"
        if (c != NULL) UISetProgress(c, 0.0f);
        c = UIWidgetFindChild(widget, 0xE4E00B6Fu);  // "Defense"
        if (c != NULL) UISetProgress(c, 0.0f);
        c = UIWidgetFindChild(widget, 0x2EADCD52u);
        if (c != NULL) UISetText(c, lbl_805F90D4, 1);
        break;
    }
    case 0x2C434515u:
        UIElement_SetVisibleFlag(0x60B15D91u, 0);
        break;
    default:
        break;
    }
}

// Frees a counted buffer {ptr, count, kind}: kinds 1 and 2 are released with BufferFree.
static inline void SirFreeBuf(u8* ptr, s32 kind) {
    if (ptr != NULL) {
        if (kind == 2) BufferFree(ptr);
        else if (kind == 1) BufferFree(ptr);
    }
}

// 0x801E5AE0
// Shuts down a SIR loader: frees the two buffers of the record at +0x7B4 and the record itself,
// releases each loaded entry (+0x7A8, count +0x7AC) through lbl_8048A8B4, then frees the list.
void SirLoaderShutdown(u8* a0) {
    s32 sp8;
    u8* rec = *(u8**)(a0 + 0x7B4);
    if (rec != NULL) {
        SirFreeBuf(*(u8**)(rec + 0x0), *(s32*)(rec + 0x8));
        *(void**)(rec + 0x0) = NULL;
        *(s32*)(rec + 0x4) = 0;
        *(s32*)(rec + 0x8) = 0;
        SirFreeBuf(*(u8**)(rec + 0xC), *(s32*)(rec + 0x14));
        *(void**)(rec + 0xC) = NULL;
        *(s32*)(rec + 0x10) = 0;
        *(s32*)(rec + 0x14) = 0;
        operator_delete(rec);
    }
    *(void**)(a0 + 0x7B4) = NULL;
    for (s32 i = 0; i < *(s32*)(a0 + 0x7AC); i++) {
        if (*(s32*)(*(u8**)(a0 + 0x7A8) + i * 4) != 0) {
            sp8 = fn_801E4E20(lbl_8048A8B4, 0 /* leftover r4 */);
            fn_801E5220(lbl_8048A8B4, &sp8);
        }
    }
    SirFreeBuf(*(u8**)(a0 + 0x7A8), *(s32*)(a0 + 0x7B0));
    *(void**)(a0 + 0x7A8) = NULL;
    *(s32*)(a0 + 0x7AC) = 0;
    *(s32*)(a0 + 0x7B0) = 0;
}

// 0x801DBF3C
// Spawns effect a1 attached to an entity (only if it has no owner at +0xA8): posts event 0x6A
// (or 0x69 with the handle at +0x30) and spawns the effect at the entity (a2 = 0) or its
// state position (a2 = 1).
void SpawnAttachedParticle(void* a0, u32 a1, u32 a2) {
    u8* e = (u8*)a0;
    if (e == NULL || *(s32*)(e + 0xA8) != 0) return;
    if (fn_801F38F8(e) != 0) {
        u8* ev = (u8*)EventPost(0x6A, e);
        if (ev != NULL) {
            *(u32*)(ev + 0x4) = a1;
            *(s32*)(ev + 0x8) = 1;
            *(u32*)(ev + 0xC) = a2;
        }
    } else if (*(s32*)(e + 0x30) != -1) {
        u8* ev = (u8*)EventPost(0x69, NULL);
        if (ev != NULL) {
            *(u32*)(ev + 0x8) = a1;
            *(s32*)(ev + 0xC) = 1;
            *(s32*)(ev + 0x4) = *(s32*)(e + 0x30);
            *(u32*)(ev + 0x10) = a2;
        }
    }
    f32 pos[4];
    if (a2 == 0) {
        pos[0] = *(f32*)(e + 0x18);
        pos[1] = *(f32*)(e + 0x1C);
        pos[2] = *(f32*)(e + 0x20);
        pos[3] = 0.0f;
        SpawnEffectAt(lbl_8047C760, a1, pos, (int)(intptr_t)(e + 0xA8));
    } else if (a2 == 1) {
        u8* st = *(u8**)(e + 0x4);
        pos[0] = *(f32*)(st + 0x0);
        pos[1] = *(f32*)(st + 0x4);
        pos[2] = *(f32*)(st + 0x8);
        pos[3] = 0.0f;
        SpawnEffectAt(lbl_8047C760, a1, pos, (int)(intptr_t)(e + 0xA8));
    }
}
// 0x800E909C
// Places an entity at path node a2 (only in phase 2, for flagged entities when gGame+0xFE0 is 0):
// moves it there, resets its velocity, refreshes it and registers a spawn record (a3/a4 as floats).
void SpawnDatabaseMonsterAtNode(void* a0, u32 a1, u32 a2, s32 a3, s32 a4) {
    u8* e = (u8*)a0;
    u8 ref[0x404];
    u8 rec[0x28];
    if (WorldGetPhase(gGame + 0xD5F0) != 2) return;
    if (!(*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x10)) return;
    if (*(s32*)(gGame + 0xFE0) != 0) return;
    u8* src = ObjDbLookup(gObjDb, (s32)a2);
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
    *(f32*)(e + 0xD4) = 0.0f;
    *(f32*)(e + 0xD8) = 0.0f;
    *(f32*)(e + 0xDC) = 0.0f;
    u8* st = *(u8**)(e + 0x4);
    *(f32*)(st + 0x0) = x;
    *(f32*)(st + 0x4) = y;
    *(f32*)(st + 0x8) = z;
    fn_80053418(e);
    if (*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x2000) {
        if ((*(s32 (**)(void*))(*(u8**)e + 0xF0))(e) == 0) fn_8004D968(e, (s32)a1, 0);
        for (s32 i = 0; i < 5; i++) {
            PartInstance* part = *(PartInstance**)(e + 0x89C + i * 4);
            if (part != NULL) PartGetCurrentHealth(part);
        }
        if (WorldGetPhase(gGame + 0xD5F0) == 2 && a3 >= 0 && a4 >= 0) {
            fn_800D6B0C((s32*)rec);
            *(s32*)(rec + 0x0) = *(s32*)(e + 0x2C);
            *(s32*)(rec + 0x8) = 0;
            *(s32*)(rec + 0x18) = -1;
            *(u32*)(rec + 0x1C) = a1;
            *(u32*)(rec + 0x4) = a2;
            *(f32*)(rec + 0xC) = (f32)a3;
            *(f32*)(rec + 0x10) = (f32)a4;
            *(u8*)(rec + 0x20) = 0;
            fn_800D70EC((s32*)rec, (f32)a3);
        }
    }
    ObjRefDtor(ref, -1);
}

// 0x800E8944
// Spawns a database monster at a node of group a2 (looked up via fn_800D4730), returning the
// node id (0 when nothing was spawned); registers a spawn record like SpawnDatabaseMonsterAtNode.
u32 SpawnDatabseMonsterAtNodeGroup(void* a0, u32 a1, s32 a2, s32 a3, s32 a4) {
    u8* e = (u8*)a0;
    u8 rec[0x28];
    s32 node = 0;
    if ((*(s32*)(*(u8**)(e + 0x4) + 0x1C) & 0x10) && *(s32*)(gGame + 0xFE0) == 0) {
        node = fn_800D4730(gObjDb, a2);
        if (node != 0) {
            SpawnDatabaseMonsterAtNode(e, a1, (u32)node, a3, a4);
            if (WorldGetPhase(gGame + 0xD5F0) == 2 && a3 >= 0 && a4 >= 0) {
                fn_800D6B0C((s32*)rec);
                *(s32*)(rec + 0x0) = *(s32*)(e + 0x2C);
                *(u32*)(rec + 0x1C) = a1;
                *(s32*)(rec + 0x18) = -1;
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
// 0x80157668
// Creates the renderer's off-screen targets (HDR red/green/blue at screen size, small 256x1
// ramps, quarter/eighth-size blur buffers, a 48x36 target), two 0x1B00-byte buffers and three
// 0x400-byte buffers, then installs the video callback fn_80157494.
void RendInitTargets(void) {
    s32 w = VideoGetWidth(&gVideo);
    s32 h = VideoGetHeight(&gVideo);
    u8* hdr = lbl_80401C30 + 0x9020;
    fn_801499F8(hdr, w, h, 0x11, 0, 2, lbl_805F9240);  // "hdrRed"
    fn_801499F8(hdr + 0x20, w, h, 0x11, 0, 2, lbl_803765C0);
    fn_801499F8(hdr + 0x40, w, h, 0x11, 0, 2, lbl_805F9248);  // "hdrBlue"
    u8* ramp = lbl_80401C30 + 0x9080;
    for (s32 i = 0; i < 2; i++, ramp += 0x60) {
        fn_801499F8(ramp, 0x100, 1, 0x16, 0, 2, lbl_803765C0 + 0xC);
        fn_801499F8(ramp + 0x20, 0x100, 1, 0x16, 0, 2, lbl_803765C0 + 0x20);
        fn_801499F8(ramp + 0x40, 0x100, 1, 0x16, 0, 2, lbl_803765C0 + 0x30);
    }
    u8* blur = lbl_80401C30 + 0x9140;
    fn_801499F8(blur, w >> 1, h >> 1, 0x16, 0, 0x12, lbl_803765C0 + 0x40);
    fn_801499F8(blur + 0x20, w >> 2, h >> 2, 0x16, 0, 0x12, lbl_803765C0 + 0x4C);
    fn_801499F8(blur + 0x40, w >> 3, h >> 3, 0x16, 0, 0x12, lbl_803765C0 + 0x58);
    fn_801499F8(lbl_80401C30 + 0x91A0, 0x30, 0x24, 0x16, 0, 2, lbl_803765C0 + 0x64);
    lbl_805FB528 = gRendMem->alloc(0x1B00, 0x10, 2, NULL, 0);
    lbl_805FB52C = gRendMem->alloc(0x1B00, 0x10, 2, NULL, 0);
    u8* slot = lbl_80401C30;
    for (s32 i = 0; i < 3; i++, slot += 4) {
        *(void**)slot = gRendMem->alloc(0x400, 0x10, 3, NULL, 0);
        fn_80156F70(i, 1);
    }
    fn_80156AC0(1);
    fn_8015D2B8(&gVideo, 1, fn_80157494);
}

} // extern C
