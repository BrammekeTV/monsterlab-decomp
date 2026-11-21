// Misc functions across 0x800Axxxx-0x801Fxxxx: selection list setup, battle HUD owners, menu
// registration, boss prop constructor/camera helpers, std::map copy, object slot setup, path follower.
#include <string.h>

#include "types.h"
#include "engine/math.h"

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

typedef u8* (*VFnRetPtr)(u8* self);                   // virtual slot returning a pointer
typedef void (*VFnSelf)(u8* self);                    // virtual slot taking only the sub-object
typedef void (*VFnSelfArg)(u8* self, u8* arg);        // virtual slot with one pointer argument
typedef void (*VFnSelfFloat)(u8* self, f32 v);        // virtual slot with one float argument
typedef void (*VFnGetVec)(f32* out, u8* self);        // virtual slot writing a vec3
typedef void (*VFnGetVecF)(f32* out, u8* self, f32 a, f32 b);  // same, with two float arguments
typedef void (*VFnVec2)(u8* self, f32* a, f32* b);    // virtual slot taking two vec3 pointers
typedef void (*VFnVec1)(u8* self, f32* a);            // virtual slot taking one vec3 pointer
typedef void (*VFnInt)(u8* self, s32 v);              // virtual slot with one int argument

extern "C++" s32 gZoneResourceSize;  // 0x805FB94C
extern "C++" u8* fn_800C42B8(u8* p);  // 0x800C42B8 (defined in small_800C.cpp)

extern "C" {
// Callees
s32 fn_800381B8(u8* p, s32 a, s32 b);                          // 0x800381B8
void FighterStatsSetOwner(u8* stats, void* owner);               // 0x8020E7B4-area helper (owner is a pointer here)
void* fn_80043910(void* a0, s32 idx, s32 kind);                  // 0x80043910
void fn_801B4160(u8* p);                                         // 0x801B4160
u8* fn_801FA7CC(u8* p, s32 a4);                                  // 0x801FA7CC
u8* fn_8006FA74(u8* p, s32 n);                                   // 0x8006FA74
void fn_800C20FC(u8* p);                                         // 0x800C20FC
void fn_800C2180(u8* p, u8* tbl, s32 n);                         // 0x800C2180
void fn_800A0FB4(u8* p);                                         // 0x800A0FB4
u8* fn_800A3128(u8* p, u32* keyp, void** outNode, u8* outLeft, u8* outAll);  // 0x800A3128
u8* fn_800A29BC(u8* tree, u8* parent, s32 left, s32 setRoot, s32* value);    // 0x800A29BC
void fn_800A11F4(u8* p);                                         // 0x800A11F4
void fn_800A71C8(u8* p, u32 a);                                  // 0x800A71C8
void fn_80075264(u8* p, s32 a);                                  // 0x80075264
f64 fn_8033F1F4(f64 x);                                          // 0x8033F1F4
void* fn_801D40E0(s32 n);                                        // 0x801D40E0
void operator_delete(void* p);                                   // 0x801D426C
s32 fn_80336BF8(u8* file, const char* fmt, ...);                 // 0x80336BF8
void fn_8033B09C(void);                                          // 0x8033B09C
void fn_800A3188(u8* tree, u8* node);                            // 0x800A3188
void fn_800A5828(u8* tree, u8* dst, u8* src, s32 side);          // 0x800A5828
s32 fn_800AEF38(u8* t);                                          // 0x800AEF38
s32 fn_800AEF98(u8* t, s32 a, u32 hash);                         // 0x800AEF98
s32 fn_800AEF48(u8* t, s32 a, s32 b, u32 hash);                  // 0x800AEF48
void fn_800B8E8C(u8* obj, s32 idx, s32 val);                     // 0x800B8E8C
void fn_800B9548(u8* obj, s32 v);                                // 0x800B9548
s32 fn_800B964C(u8* obj);                                        // 0x800B964C
void fn_800B9654(u8* obj, f32 v);                                // 0x800B9654
void fn_800B991C(u8* obj, s32 v);                                // 0x800B991C
void fn_800B9994(u8* obj, f32 v);                                // 0x800B9994
void fn_800B999C(u8* obj, s32 a, s32 b);                         // 0x800B999C
u8* fn_800B6328(u8* base, u8* slot, s32 idx);                    // 0x800B6328
s32 fn_800BCA44(u8* obj, s32 idx);                              // 0x800BCA44
void fn_800BD948(u8* obj, s32 idx);                              // 0x800BD948
void fn_800BE3A4(u8* obj);                                       // 0x800BE3A4
void fn_800BF6FC(u8* obj);                                       // 0x800BF6FC
void fn_800BFBB0(u8* list, u8* obj, s32 idx);                    // 0x800BFBB0
void ScopedLockCtor(ScopedLock* l);                              // 0x801F5384
void ScopedLockDtor(ScopedLock* l, s32 flags);                   // 0x801F53B0
s32 RaySceneTest(void* ray);                                     // 0x80070458 (ignores its argument)
s32 fn_800CD490(u8* t, s32 v);                                   // 0x800CD490
void fn_800C8E54(u8* obj, u8* dst, s32 idx);                     // 0x800C8E54
s32 fn_800CDC9C(u8* p);                                          // 0x800CDC9C
void fn_800C75E8(u8* a, u8* b);                                  // 0x800C75E8
void fn_800CD9F0(u8* a, f32* b);                                 // 0x800CD9F0
s32 fn_800CC854(u8* obj, u8* b);                                 // 0x800CC854
void fn_800D6A24(u8** p);                                        // 0x800D6A24
const char* PartSlotName(s32 slot);                              // 0x8020E7B4
char* fn_80040EFC(s32 idx, u8* buf);                             // 0x80040EFC
void fn_801B381C(u8* menu, const char* name, void* a, void* b, void* c, void* d, void* e, void* user);  // 0x801B381C
u8* fn_8003F91C(u8* p, u8* owner);                               // 0x8003F91C
u8* fn_8020BCEC(u8* p, u8* owner);                               // 0x8020BCEC
u8* fn_80040090(u8* p, u8* owner);                               // 0x80040090
void fn_80040638(u8* p, u8* owner);                              // 0x80040638
u8* fn_8004421C(u8* p, s32 arg1);                                  // 0x8004421C
void fn_8020F498(u8* p, s32 a2);                                 // 0x8020F498
u8* fn_800444FC(u8* p, s32 a2);                                  // 0x800444FC

// Menu callbacks registered by fn_80042E04
void fn_800440E0(u8* p);   // 0x800440E0
void fn_8004415C(u8* p);   // 0x8004415C (second entry point at +8)
void fn_80044098(u8* p);   // 0x80044098
void fn_800433F8(void);    // 0x800433F8
void fn_80043824(u8* p);   // 0x80043824
void fn_80043800(u8* p);   // 0x80043800
void fn_800436A0(u8* p);   // 0x800436A0
void fn_80044010(u8* p);   // 0x80044010
void fn_8004405C(u8* p);   // 0x8004405C
u32 fn_80044078(u8* p);    // 0x80044078
void fn_80043E94(u8* p);   // 0x80043E94
void fn_80043400(u8* p);   // 0x80043400
void fn_800434C8(u8* p);   // 0x800434C8
void fn_80043500(u8* p);   // 0x80043500
void fn_80043538(u8* p);   // 0x80043538
void fn_80043964(u8* p);   // 0x80043964
void fn_80043CE8(u8* p);   // 0x80043CE8
void fn_80043CF0(u8* p);   // 0x80043CF0
void fn_80043CF8(u8* p);   // 0x80043CF8

// Data
extern s32 lbl_8034760C[];   // 0x8034760C
extern s32 lbl_80347698[];   // 0x80347698
extern s32 lbl_805F8538;     // 0x805F8538
extern void* lbl_805FB058;   // 0x805FB058
extern u8 lbl_8047C7D8[];    // 0x8047C7D8
extern u8* lbl_803C51D8[];   // 0x803C51D8 (table of object pointers)
extern u8* gBattle;
extern u8 gWorldState[];     // 0x803E6D00
extern u8 lbl_80386428[];    // 0x80386428 (vtable)
extern u8 lbl_8035CF88[];    // 0x8035CF88 (vtable)
extern u8 lbl_8035D020[];    // 0x8035D020 (vtable)
extern u8 lbl_8035D080[];    // 0x8035D080 (vtable)
extern u8 lbl_803C6C08[];    // 0x803C6C08
extern u8 lbl_803C4298[];    // 0x803C4298
extern u8* lbl_805FB158;     // 0x805FB158
extern u8* lbl_805FB160;     // 0x805FB160
extern u8* lbl_805FB164;     // 0x805FB164
extern u8 __files[];         // 0x803B4978
extern char lbl_803587F0[];  // 0x803587F0 (menu label strings)
extern char lbl_805F85A0[];  // 0x805F85A0 (" ")
extern u8* lbl_805FB078[];   // 0x805FB078
extern u8* lbl_805FB080[];   // 0x805FB080
extern u8* lbl_805FB088;     // 0x805FB088
extern f32 lbl_80346108[];   // 0x80346108
extern f32 lbl_803460E8[];   // 0x803460E8
extern f32 lbl_803460F8[];   // 0x803460F8
extern s32 lbl_80346118[];   // 0x80346118
extern s32 lbl_80346128[];   // 0x80346128
extern s32 lbl_80346138[];   // 0x80346138
extern f32 lbl_80346158[];   // 0x80346158
extern f32 lbl_80346178[];   // 0x80346178
extern f32 lbl_80346198[];   // 0x80346198
}

// Tracked difficulty-style index (0..3) from RaySceneTest, clamped.
static inline s32 ClampedRayScene(void) {
    s32 v = RaySceneTest(NULL);
    if (v < 0) return 0;
    if (v > 3) return 3;
    return v;
}

// Pointer to the target point of a path follower state (idx -4/-3 pick the end/start node).
static inline u8* PathTarget(u8* s) {
    s32 idx = *(s32*)(s + 0xC);
    if (idx == -4) return *(u8**)(*(u8**)(s + 0x10) + 8) + 0x34;
    if (idx == -3) return *(u8**)(*(u8**)(s + 0x10) + 4) + 0x34;
    u8* base = *(u8**)(s + 0x10);
    u8* tgt = base + 0x18;
    s32 n = *(s32*)(base + 0xD8);
    s32 c = (idx < 0) ? 0 : ((idx > n) ? n : idx);
    if (idx == c) tgt += idx * 0x10;
    return tgt;
}

// Post-order delete of the top four levels of a std::map tree (root = depth 0); children of the
// depth-3 nodes go through fn_800A3188.
static void EraseSlice(u8* tree, u8* n, s32 depth) {
    if (n == NULL) return;
    for (s32 k = 0; k < 2; k++) {
        u8* c = *(u8**)(n + k * 4);
        if (depth < 3) {
            EraseSlice(tree, c, depth + 1);
        } else if (c != NULL) {
            fn_800A3188(tree, c);
        }
    }
    operator_delete(n);
}

extern "C" {

// 0x80037FB4
// Sets up the selection list for a mode: mode 4/2 get one fixed entry, modes 0/1/3 get a count
// clamped from a per-character table, then mode 3 additionally calls fn_800381B8 for table entries
// that fit the count.
void fn_80037FB4(u8* p) {
    s32 mode = *(s32*)(p + 0x9A8);
    *(s32*)(p + 0xA40) = -1;
    if (mode == 4) {
        *(s32*)(p + 0x9B4) = 9;
        *(s32*)(p + 0x9F0) = 8;
        *(u8*)(p + 0xA2C) = 0;
        *(s32*)(p + 0xA3C) = 1;
        return;
    }
    if (mode == 2) {
        *(s32*)(p + 0x9B4) = 0;
        *(s32*)(p + 0x9F0) = 0;
        *(u8*)(p + 0xA2C) = 0;
        *(s32*)(p + 0xA3C) = 1;
        return;
    }
    s32 n = lbl_8034760C[*(s32*)(p + 0x164)];
    s32 maxN;
    switch (mode) {
    case 0: maxN = 0xC; break;
    case 1: maxN = 4; break;
    case 3: maxN = 0xA; break;
    default: maxN = -1; break;
    }
    s32 count;
    if (maxN < 0) {
        count = 0;
    } else if (n < 0) {
        count = 0;
    } else {
        count = (n > maxN) ? maxN : n;
    }
    *(s32*)(p + 0xA3C) = count;
    for (s32 i = 0; i < *(s32*)(p + 0xA3C); i++) {
        *(s32*)(p + 0x9B4 + i * 4) = i;
        *(s32*)(p + 0x9F0 + i * 4) = i;
        *(u8*)(p + 0xA2C + i) = 0;
    }
    count = *(s32*)(p + 0xA3C);
    if (*(s32*)(p + 0x9A8) == 3) {
        for (s32 k = 0; k < 3; k++) {
            s32 v = lbl_80347698[k];
            if (v < count) fn_800381B8(p, v, 0)/*r5 leftover in asm*/;
        }
    }
}

// 0x80040F44
// Battle HUD owner refresh: finds the lowest set bit (1, 2, 4) in lbl_805F8538 and clears it; for
// bit 2 it rebuilds both fighters' owner/HUD slots. With no bit set it marks all three bits and
// rebinds both fighters to the default owners.
void fn_80040F44(u8* p) {
    s32 flags = lbl_805F8538;
    u32 bit = 1;
    while (bit < 7) {
        if (flags & bit) {
            lbl_805F8538 = lbl_805F8538 & ~bit;
            if (bit == 2) {
                *(u8**)(gBattle + 0xA54) = NULL;
                *(u8**)(gBattle + 0xA58) = NULL;
                fn_801B4160(lbl_8047C7D8);
                FighterStatsSetOwner(*(u8**)(gBattle + 0xA00) + 0x20, lbl_803C51D8[4]);
                *(void**)(gBattle + 0xA54) = fn_80043910(lbl_805FB058, 0, 0);
                FighterStatsSetOwner(*(u8**)(gBattle + 0xA04) + 0x20, lbl_803C51D8[3]);
                *(void**)(gBattle + 0xA58) = fn_80043910(lbl_805FB058, 1, 1);
            }
            return;
        }
        bit <<= 1;
    }
    lbl_805F8538 = 7;
    FighterStatsSetOwner(*(u8**)(gBattle + 0xA00) + 0x20, lbl_803C51D8[0]);
    FighterStatsSetOwner(*(u8**)(gBattle + 0xA04) + 0x20, lbl_803C51D8[2]);
    for (s32 i = 0; i < 2; i++) {
        *(u8**)(gBattle + 0xA54 + i * 4) = NULL;
        *(void**)(gBattle + 0xA54 + i * 4) = fn_80043910(lbl_805FB058, i, 0);
    }
}

// 0x80042E04
// Builds the battle debug menu: registers the per-fighter option rows (state, pad, mode, part
// rows) on the menu at +0x260, then allocates and constructs the HUD/helper objects stored in
// lbl_803C51D8, lbl_805FB078/80 and lbl_805FB088.
void fn_80042E04(u8* p) {
    char buf[0x40];
    u8* menu = *(u8**)(p + 0x260);
    strncpy((char*)menu, lbl_803587F0, 0x5F);
    *(u8*)(menu + 0x5F) = 0;
    for (s32 i = 0; i < 2; i++) {
        fn_80040EFC(i, (u8*)buf);
        strcat(buf, lbl_803587F0 + 0x20);
        s32 st = *(s32*)(p + 0x250 + i * 8);
        if (st != 0 && st != 1) {
            *(u8*)(p + 0x254 + i * 8) = 0;
            *(u8*)(p + 0x255 + i * 8) = 0;
        }
        *(s32*)(p + 0x250 + i * 8) = i;
        fn_801B381C(*(u8**)(p + 0x260), buf, (void*)fn_800440E0, (void*)fn_8004415C,
                    (void*)((u8*)fn_8004415C + 8), (void*)fn_80044098, NULL, p + 0x250 + i * 8);
    }
    fn_801B381C(*(u8**)(p + 0x260), lbl_803587F0 + 0x34, NULL, NULL, NULL, (void*)fn_800433F8, NULL, p);
    fn_801B381C(*(u8**)(p + 0x260), lbl_803587F0 + 0x54, NULL, NULL, NULL, (void*)fn_80043824, NULL, p);
    for (s32 i = 0; i < 2; i++) {
        fn_80040EFC(i, (u8*)buf);
        strcat(buf, lbl_803587F0 + 0x70);
        *(u8**)(p + 0x230 + i * 8) = p;
        *(s32*)(p + 0x234 + i * 8) = i;
        fn_801B381C(*(u8**)(p + 0x260), buf, (void*)fn_80043800, (void*)fn_800436A0, (void*)fn_800436A0,
                    NULL, NULL, p + 0x230 + i * 8);
    }
    for (s32 i = 0; i < 2; i++) {
        fn_80040EFC(i, (u8*)buf);
        strcat(buf, lbl_803587F0 + 0x88);
        s32 v = *(s32*)(p + 0x244 + i * 8);
        if (v < 0 || v >= 0x11) *(s32*)(p + 0x244 + i * 8) = 0;
        *(s32*)(p + 0x240 + i * 8) = i;
        fn_801B381C(*(u8**)(p + 0x260), buf, (void*)fn_80044010, (void*)fn_8004405C, (void*)fn_80044078,
                    (void*)fn_80043E94, NULL, p + 0x240 + i * 8);
    }
    fn_801B381C(*(u8**)(p + 0x260), lbl_803587F0 + 0xA4, NULL, NULL, NULL, NULL, NULL, NULL);
    for (s32 fi = 0; fi < 2; fi++) {
        for (s32 part = 0; part < 5; part++) {
            u8* e = p + fi * 0x8C + part * 0x1C;
            *(u8**)(e + 0x0) = p;
            *(s32*)(e + 0x4) = fi;
            *(s32*)(e + 0x8) = part;
            u8* fighter = *(u8**)(gBattle + 0xA00 + fi * 4);
            u8* partObj = *(u8**)(fighter + part * 4);
            if (partObj != NULL) {
                *(s32*)(e + 0xC) = **(s32**)partObj;
                fn_80040EFC(fi, (u8*)buf);
                strcat(buf, lbl_805F85A0);
                strcat(buf, PartSlotName(part));
                fn_801B381C(*(u8**)(p + 0x260), buf, (void*)fn_80043400, (void*)fn_800434C8,
                            (void*)fn_80043500, (void*)fn_80043538, NULL, e);
            }
        }
    }
    fn_801B381C(*(u8**)(p + 0x260), lbl_803587F0 + 0xCC, NULL, NULL, NULL, NULL, NULL, NULL);
    for (s32 fi = 0; fi < 2; fi++) {
        for (s32 part = 0; part < 5; part++) {
            u8* e = p + fi * 0x8C + part * 0x1C;
            *(u8**)(e + 0x118) = p;
            *(s32*)(e + 0x11C) = fi;
            *(s32*)(e + 0x120) = part;
            *(s32*)(e + 0x124) = *(s32*)(e + 0xC);
            u8* fighter = *(u8**)(gBattle + 0xA00 + fi * 4);
            u8* partObj = *(u8**)(fighter + part * 4);
            // (partObj + 0x28) is never null once partObj is non-null, so the asm's second test is moot.
            if (partObj != NULL) {
                *(s32*)(e + 0x124) = **(s32**)partObj;
                u8* info = *(u8**)(partObj + 0x28);
                s32 a = *(s32*)(info + 0x20);
                s32 b = *(s32*)(info + 0x24);
                if (a != -1) {
                    *(s32*)(e + 0x128) = a;
                    *(s32*)(e + 0x12C) = -1;
                    *(s32*)(e + 0x130) = 1;
                } else if (b != -1) {
                    *(s32*)(e + 0x128) = -1;
                    *(s32*)(e + 0x12C) = b;
                    *(s32*)(e + 0x130) = -1;
                } else {
                    *(s32*)(e + 0x128) = -1;
                    *(s32*)(e + 0x12C) = -1;
                    *(s32*)(e + 0x130) = 0;
                }
                fn_80040EFC(fi, (u8*)buf);
                strcat(buf, lbl_805F85A0);
                strcat(buf, PartSlotName(part));
                fn_801B381C(*(u8**)(p + 0x260), buf, (void*)fn_80043964, (void*)fn_80043CE8,
                            (void*)fn_80043CF0, (void*)fn_80043CF8, NULL, e + 0x118);
            }
        }
    }
    u8* f0 = *(u8**)(gBattle + 0xA00);
    u8* f1 = *(u8**)(gBattle + 0xA04);
    u8* o = (u8*)fn_801D40E0(0x54);
    if (o != NULL) o = fn_8003F91C(o, f0);
    lbl_803C51D8[0] = o;
    o = (u8*)fn_801D40E0(0x50);
    if (o != NULL) o = fn_8020BCEC(o, f0);
    lbl_803C51D8[1] = o;
    o = (u8*)fn_801D40E0(0x54);
    if (o != NULL) o = fn_8003F91C(o, f1);
    lbl_803C51D8[2] = o;
    o = (u8*)fn_801D40E0(0x50);
    if (o != NULL) o = fn_8020BCEC(o, f1);
    lbl_803C51D8[3] = o;
    o = (u8*)fn_801D40E0(0x50);
    if (o != NULL) o = fn_80040090(o, f0);
    lbl_803C51D8[4] = o;
    // The asm stores r3 after the constructor call; the constructor leaves the object in r3.
    o = (u8*)fn_801D40E0(0x6C);
    if (o != NULL) fn_80040638(o, f1);
    lbl_803C51D8[5] = o;
    for (s32 i = 0; i < 2; i++) {
        o = (u8*)fn_801D40E0(0x4D4);
        if (o != NULL) o = fn_8004421C(o, 0)/*arg1 leftover*/;
        lbl_805FB078[i] = o;
        o = (u8*)fn_801D40E0(0x48);
        if (o != NULL) fn_8020F498(o, 0);
        lbl_805FB080[i] = o;
    }
    o = (u8*)fn_801D40E0(0x48);
    if (o != NULL) o = fn_800444FC(o, 0);
    lbl_805FB088 = o;
}

// 0x8009F8E8
// Constructor of the saw-blade boss prop: base init (fn_8006FA74), vtables and fields of the
// embedded sub-objects, then registers the zone objects of kinds A/B in the tree at +0xCE8 and
// clears per-zone flags before the final camera/offset setup.
u8* fn_8009F8E8(u8* p) {
    struct KeyNode { u32 key; u8* node; };
    f32 vec[3];
    fn_8006FA74(p, 2);
    u8* sub = p + 0x8C;
    *(void**)(p + 4) = lbl_8035CF88;
    fn_800C42B8(sub);
    *(void**)(sub + 0xD4) = lbl_8035D020;
    *(s32*)(sub + 0xD0) = 1;
    u8* list = p + 0xC74;
    fn_800C20FC(list);
    *(void**)(list + 0xC) = lbl_8035D080;
    *(s32*)(list + 0x10) = 0;
    *(s32*)(list + 0x14) = 0;
    *(u8**)(p + 0xC8C) = NULL;
    *(f32*)(p + 0xC90) = 0.0f;       // lbl_805FDE40
    *(u8**)(p + 0xCAC) = NULL;
    *(f32*)(p + 0xCCC) = 0.0f;
    *(s32*)(p + 0xCD4) = 0;
    *(s32*)(p + 0xCD8) = 0;
    *(s32*)(p + 0xCDC) = 0;
    *(u8**)(p + 0xCE4) = p + 0xCD8 + 4;
    *(s32*)(p + 0xCE8) = 0;
    *(s32*)(p + 0xCEC) = 0;
    *(u8**)(p + 0xCF0) = p + 0xCE8 + 4;
    *(f32*)(p + 0xCF4) = 50.0f;      // lbl_805FDE44
    lbl_805FB158 = p;
    lbl_805FB160 = p + 0xCD8;
    lbl_805FB164 = p + 0xCE8;
    fn_800A0FB4(p);
    u8* world = *(u8**)gWorldState;
    u8* zone = *(u8**)(world + 0x1A4) + gZoneResourceSize;
    for (s32 j = 0; j < *(s32*)(zone + 0x98); j++) {
        u8* node = *(u8**)(*(u8**)(zone + 0x12C) + j * 4);
        u8* o = *(u8**)(node + 0x9C);
        u32 tag = (o != NULL) ? *(u32*)(o + 8) : 0;
        switch (tag) {
        case 0xAB868DB8u:
        case 0x8D6957B0u: {
            u8* t = (*(VFnRetPtr*)(*(u8**)node + 0xC))(node);
            if (t != NULL) {
                t = (*(VFnRetPtr*)(*(u8**)node + 0xC))(node);
                *(u32*)(t + 8) = *(u32*)(t + 8) & ~2u;
            }
            u8* sh = *(u8**)(node + 4);
            *(u32*)(sh + 0x20) = *(u32*)(sh + 0x20) & ~3u;
            o = *(u8**)(node + 0x9C);
            KeyNode kn;
            kn.key = (o != NULL) ? *(u32*)(o + 8) : 0;
            kn.node = node;
            void* outNode;
            u8 left, all;
            u8* parent = fn_800A3128(p + 0xCE8, &kn.key, &outNode, &left, &all);
            fn_800A29BC(p + 0xCE8, parent, left, all, (s32*)&kn);
            break;
        }
        case 0x677E3F1Bu: {
            u8* t = (*(VFnRetPtr*)(*(u8**)node + 0xC))(node);
            if (t != NULL) {
                t = (*(VFnRetPtr*)(*(u8**)node + 0xC))(node);
                *(u32*)(t + 8) = *(u32*)(t + 8) & ~2u;
            }
            u8* sh = *(u8**)(node + 4);
            *(u32*)(sh + 0x20) = *(u32*)(sh + 0x20) & ~2u;
            o = *(u8**)(node + 0x9C);
            KeyNode kn;
            kn.key = (o != NULL) ? *(u32*)(o + 8) : 0;
            kn.node = node;
            void* outNode;
            u8 left, all;
            u8* parent = fn_800A3128(p + 0xCE8, &kn.key, &outNode, &left, &all);
            fn_800A29BC(p + 0xCE8, parent, left, all, (s32*)&kn);
            break;
        }
        default:
            break;
        }
    }
    for (s32 z = 0; z <= *(s32*)(world + 4); z++) {
        u8 k = 0;
        u8* entries = *(u8**)(*(u8**)(*(u8**)gWorldState + 0x1A4) + z * gZoneResourceSize + 0xA0);
        while (k < *(u8*)(*(u8**)(*(u8**)gWorldState + 0x1A4) + z * gZoneResourceSize + 0xA4)) {
            u8* ent = entries + k * 0x4C;
            k++;
            *(u32*)(ent + 0x30) = *(u32*)(ent + 0x30) & ~1u;
        }
    }
    fn_800A71C8(*(u8**)(p + 0xC8C), 0x2397C385u);
    fn_800A11F4(p);
    *(s32*)(p + 0xC84) = 1;
    fn_800C2180(p + 0xC74, lbl_803C6C08, 3);
    (*(VFnSelfArg*)(*(u8**)(p + 0x160) + 0xC))(p + 0x8C, p + 0xC74);
    (*(VFnSelfFloat*)(*(u8**)(p + 0x160) + 0x14))(p + 0x8C, 0.0f);
    (*(VFnSelf*)(*(u8**)(p + 0x160) + 0x20))(p + 0x8C);
    (*(VFnGetVec*)(*(u8**)(p + 0x160) + 0x24))(vec, p + 0x8C);
    f32* tgt = *(f32**)(*(u8**)(p + 0xCAC) + 4);
    *(f32*)(p + 0xCB0) = tgt[0] - vec[0];
    *(f32*)(p + 0xCB4) = tgt[1] - vec[1];
    *(f32*)(p + 0xCB8) = tgt[2] - vec[2];
    *(s32*)(p + 0xCF8) = -1;
    *(s32*)(p + 0xCFC) = -1;
    fn_80075264(p, 0x3A2);
    *(s32*)(p + 0x8) = 0x2A8;
    *(s32*)(p + 0xC) = 0x2A9;
    *(u32*)(p + 0x30) = 0xFAB3B8A8u;
    *(u32*)(p + 0x34) = 0x63BAE912u;
    return p;
}

// 0x800A0BAC
// Aims the boss prop's camera pair: takes the sub-object's direction vector, computes its angle
// to the +Y axis (signed by the +X dot), rotates two reference offsets by it in the XY plane,
// translates them by the sub-object position, stores them at +0xC94/+0xCA0 and hands them to the
// two global objects (slots 0x2C/0x1C on the first, 0x98 on the second).
void fn_800A0BAC(u8* p) {
    f32 a[3] = { 0.0f, -11.0f, 8.0f };   // sp44
    f32 b[3] = { 0.0f, 8.0f, 0.0f };     // sp38
    f32 v[3];                            // sp2C
    f32 t[3];                            // sp8
    f32 up[3] = { 0.0f, 1.0f, 0.0f };
    f32 fwd[3] = { 1.0f, 0.0f, 0.0f };
    // f1/f2 still hold -11.0 / 0.0 here
    (*(VFnGetVecF*)(*(u8**)(p + 0x160) + 0x28))(v, p + 0x8C, -11.0f, 0.0f);
    f32 lenV2 = v[2] * v[2] + (v[0] * v[0] + v[1] * v[1]);
    f32 dot = v[2] * 0.0f + (v[0] * 0.0f + v[1] * up[1]);
    f32 lenV = (lenV2 > 0.0f) ? sqrtf(lenV2) : 0.0f;
    f32 lenU2 = up[2] * up[2] + (up[0] * up[0] + up[1] * up[1]);
    f32 lenU = (lenU2 > 0.0f) ? sqrtf(lenU2) : 0.0f;
    f32 den = lenV * lenU;
    f32 ang = 0.0f;
    if (!(den <= 0.0f)) {
        f32 c = dot / den;
        c = (c < -1.0f) ? -1.0f : ((c > 1.0f) ? 1.0f : c);
        ang = 0.63661975f * (f32)fn_8033F1F4((f64)c);
    }
    if (v[0] * fwd[0] + v[1] * fwd[1] + v[2] * fwd[2] > 0.0f) ang = -ang;
    f32 th = 1.5707964f * ang;
    f32 c1 = cosf(th);
    f32 s1 = sinf(th);
    f32 nbx = b[0] * c1 + b[1] * (-s1);
    f32 nby = b[1] * c1 + b[0] * s1;
    b[0] = nbx;
    b[1] = nby;
    th = 1.5707964f * ang;
    f32 c2 = cosf(th);
    f32 s2 = sinf(th);
    f32 nax = a[0] * c2 + a[1] * (-s2);
    f32 nay = a[1] * c2 + a[0] * s2;
    a[1] = nay;
    a[0] = nax;
    (*(VFnSelf*)(*(u8**)(p + 0x160) + 0x20))(p + 0x8C);
    (*(VFnGetVec*)(*(u8**)(p + 0x160) + 0x24))(t, p + 0x8C);
    b[0] += t[0];
    b[1] += t[1];
    b[2] += t[2];
    a[0] += t[0];
    a[1] += t[1];
    a[2] += t[2];
    *(f32*)(p + 0xC94) = a[0];
    *(f32*)(p + 0xC98) = a[1];
    *(f32*)(p + 0xC9C) = a[2];
    *(f32*)(p + 0xCA0) = b[0];
    *(f32*)(p + 0xCA4) = b[1];
    *(f32*)(p + 0xCA8) = b[2];
    u8* g1 = *(u8**)(lbl_803C4298 + 4);
    (*(VFnVec2*)(*(u8**)g1 + 0x2C))(g1, a, b);
    (*(VFnVec1*)(*(u8**)g1 + 0x1C))(g1, a);
    u8* g2 = *(u8**)(lbl_803C4298 + 8);
    (*(VFnSelfFloat*)(*(u8**)g2 + 0x98))(g2, 0.0f);
}

// 0x800A53B8
// std::map assignment from another map: copies the source root node and its subtrees into a
// temporary tree (fn_800A5828), swaps it into *dst, then erases the old contents. The erase is
// unrolled three levels deep in the original; deeper subtrees go through fn_800A3188.
void fn_800A53B8(u8* dst, u8* src) {
    if (*(u8**)(src + 4) == NULL) return;
    u8* node = (u8*)fn_801D40E0(0x14);
    if (node == NULL) {
        fn_80336BF8(__files + 0xA0, "Memory allocation failure");
        fn_8033B09C();
    }
    u8 tmp[0x10];
    u8* srcRoot = *(u8**)(src + 4);
    // the value copy at node + 0xC is guarded by a never-null address test in the asm
    *(s32*)(node + 0xC) = *(s32*)(srcRoot + 0xC);
    *(s32*)(node + 0x10) = *(s32*)(srcRoot + 0x10);
    *(s32*)(tmp + 0) = 0;
    *(u8**)(tmp + 4) = NULL;
    *(u8**)(tmp + 8) = tmp + 4;
    *(u8**)(node + 4) = NULL;
    *(u8**)(node + 0) = NULL;
    *(uintptr_t*)(node + 8) = (uintptr_t)(tmp + 4) | (*(uintptr_t*)(node + 8) & 1);
    if (*(u32*)(*(u8**)(src + 4) + 8) & 1) {
        *(uintptr_t*)(node + 8) = *(uintptr_t*)(node + 8) | 1;
    } else {
        *(uintptr_t*)(node + 8) = *(uintptr_t*)(node + 8) & ~(uintptr_t)1;
    }
    *(s32*)(tmp + 0) = 1;
    *(u8**)(tmp + 4) = node;
    *(u8**)(tmp + 8) = node;
    srcRoot = *(u8**)(src + 4);
    if (*(s32*)(srcRoot + 4) != 0) fn_800A5828(tmp, node, srcRoot, 0);
    srcRoot = *(u8**)(src + 4);
    if (*(s32*)(srcRoot + 0) != 0) fn_800A5828(tmp, *(u8**)(tmp + 4), srcRoot, 1);
    if (dst != tmp) {
        s32 w0 = *(s32*)(dst + 0);
        *(s32*)(dst + 0) = *(s32*)(tmp + 0);
        *(s32*)(tmp + 0) = w0;
        u8* w1 = *(u8**)(dst + 4);
        *(u8**)(dst + 4) = *(u8**)(tmp + 4);
        *(u8**)(tmp + 4) = w1;
        u8* w2 = *(u8**)(dst + 8);
        *(u8**)(dst + 8) = *(u8**)(tmp + 8);
        *(u8**)(tmp + 8) = w2;
        u8* r = *(u8**)(dst + 4);
        if (r != NULL) {
            *(uintptr_t*)(r + 8) = (uintptr_t)(dst + 4) | (*(uintptr_t*)(r + 8) & 1);
        } else {
            *(u8**)(dst + 8) = dst + 4;
        }
        r = *(u8**)(tmp + 4);
        if (r != NULL) {
            *(uintptr_t*)(r + 8) = (uintptr_t)(tmp + 4) | (*(uintptr_t*)(r + 8) & 1);
        } else {
            *(u8**)(tmp + 8) = tmp + 4;
        }
    }
    EraseSlice(tmp, *(u8**)(tmp + 4), 0);
}

// 0x800ADDB8
// Registers a script move's keyframe/segment list for a given move hash: stores the move hash and
// state 1 (0 for one move, 2 for another), then chains fn_800AEF98/fn_800AEF48 entries over the
// handle slots at +0x334..+0x35C with the move's two segment hashes.
void fn_800ADDB8(u8* p, u32 hash) {
    *(u32*)(p + 0x268) = hash;
    *(s32*)(p + 0x264) = 1;
    u8* t = p + 0x27C;
    fn_800AEF38(t);
    if (hash == 0xC2FDDCDCu) {
        *(s32*)(p + 0x264) = 0;
        return;
    }
    if (hash == 0xB5FAEC4Au) {
        fn_800AEF98(t, *(s32*)(p + 0x35C), 0xF33A3517u);
        fn_800AEF48(t, *(s32*)(p + 0x354), *(s32*)(p + 0x350), 0xF33A3517u);
        fn_800AEF48(t, *(s32*)(p + 0x350), *(s32*)(p + 0x34C), 0xF33A3517u);
        fn_800AEF48(t, *(s32*)(p + 0x34C), *(s32*)(p + 0x338), 0xF33A3517u);
        fn_800AEF98(t, *(s32*)(p + 0x338), 0xF33A3517u);
        return;
    }
    if (hash == 0x2CF3BDF0u) {
        fn_800AEF98(t, *(s32*)(p + 0x35C), 0xE4BE349Fu);
        fn_800AEF48(t, *(s32*)(p + 0x354), *(s32*)(p + 0x350), 0xE4BE349Fu);
        fn_800AEF48(t, *(s32*)(p + 0x350), *(s32*)(p + 0x34C), 0xE4BE349Fu);
        fn_800AEF48(t, *(s32*)(p + 0x34C), *(s32*)(p + 0x348), 0xE4BE349Fu);
        fn_800AEF98(t, *(s32*)(p + 0x348), 0xA59B7005u);
        fn_800AEF48(t, *(s32*)(p + 0x348), *(s32*)(p + 0x344), 0xA59B7005u);
        fn_800AEF48(t, *(s32*)(p + 0x344), *(s32*)(p + 0x340), 0xA59B7005u);
        fn_800AEF48(t, *(s32*)(p + 0x340), *(s32*)(p + 0x33C), 0xA59B7005u);
        fn_800AEF48(t, *(s32*)(p + 0x33C), *(s32*)(p + 0x338), 0xA59B7005u);
        fn_800AEF98(t, *(s32*)(p + 0x338), 0xA59B7005u);
        return;
    }
    if (hash == 0x39214CD8u) {
        fn_800AEF98(t, *(s32*)(p + 0x35C), 0xF33A3517u);
        fn_800AEF98(t, *(s32*)(p + 0x348), 0xF33A3517u);
        fn_800AEF48(t, *(s32*)(p + 0x348), *(s32*)(p + 0x344), 0xF33A3517u);
        fn_800AEF48(t, *(s32*)(p + 0x344), *(s32*)(p + 0x340), 0xF33A3517u);
        fn_800AEF48(t, *(s32*)(p + 0x340), *(s32*)(p + 0x33C), 0xF33A3517u);
        fn_800AEF48(t, *(s32*)(p + 0x33C), *(s32*)(p + 0x338), 0xF33A3517u);
        fn_800AEF98(t, *(s32*)(p + 0x338), 0xF33A3517u);
        return;
    }
    if (hash == 0x5BF48D66u) {
        *(s32*)(p + 0x264) = 2;
        fn_800AEF98(t, *(s32*)(p + 0x35C), 0x78BD9EA9u);
        fn_800AEF48(t, *(s32*)(p + 0x354), *(s32*)(p + 0x350), 0x78BD9EA9u);
        fn_800AEF48(t, *(s32*)(p + 0x350), *(s32*)(p + 0x34C), 0x78BD9EA9u);
        fn_800AEF48(t, *(s32*)(p + 0x34C), *(s32*)(p + 0x348), 0x78BD9EA9u);
        fn_800AEF98(t, *(s32*)(p + 0x348), 0xA59B7005u);
        fn_800AEF48(t, *(s32*)(p + 0x348), *(s32*)(p + 0x344), 0xA59B7005u);
        fn_800AEF48(t, *(s32*)(p + 0x344), *(s32*)(p + 0x340), 0xA59B7005u);
        fn_800AEF48(t, *(s32*)(p + 0x340), *(s32*)(p + 0x33C), 0xA59B7005u);
        fn_800AEF48(t, *(s32*)(p + 0x33C), *(s32*)(p + 0x338), 0xA59B7005u);
        fn_800AEF98(t, *(s32*)(p + 0x338), 0xA59B7005u);
        fn_800AEF48(t, *(s32*)(p + 0x338), *(s32*)(p + 0x334), 0xA59B7005u);
        fn_800AEF98(t, *(s32*)(p + 0x334), 0xA59B7005u);
        return;
    }
    if (hash == 0xF27D9F7Du) {
        fn_800AEF98(t, *(s32*)(p + 0x35C), 0x5AFD6859u);
        fn_800AEF98(t, *(s32*)(p + 0x348), 0x5AFD6859u);
        fn_800AEF48(t, *(s32*)(p + 0x348), *(s32*)(p + 0x344), 0x5AFD6859u);
        fn_800AEF48(t, *(s32*)(p + 0x344), *(s32*)(p + 0x340), 0x5AFD6859u);
        fn_800AEF48(t, *(s32*)(p + 0x340), *(s32*)(p + 0x33C), 0x5AFD6859u);
        fn_800AEF48(t, *(s32*)(p + 0x33C), *(s32*)(p + 0x338), 0x5AFD6859u);
        fn_800AEF98(t, *(s32*)(p + 0x338), 0xA59B7005u);
        fn_800AEF48(t, *(s32*)(p + 0x338), *(s32*)(p + 0x334), 0xA59B7005u);
        fn_800AEF98(t, *(s32*)(p + 0x334), 0xA59B7005u);
        return;
    }
    if (hash == 0x6B74CEC7u) {
        fn_800AEF98(t, *(s32*)(p + 0x35C), 0xF33A3517u);
        fn_800AEF98(t, *(s32*)(p + 0x338), 0xF33A3517u);
        fn_800AEF48(t, *(s32*)(p + 0x338), *(s32*)(p + 0x334), 0xF33A3517u);
        fn_800AEF98(t, *(s32*)(p + 0x334), 0xF33A3517u);
    }
}

// 0x800B33CC
// Assigns the five slot objects at +0xA8 (stride 0x14) to difficulty-limited kinds 0/1/2 (counts
// capped by per-level tables), sets each object's mode and pose from its kind, forces at least one
// kind-1 object if none was chosen, then clears the per-slot state and the counters at +0x160.
void fn_800B33CC(u8* p) {
    s32 c0 = 0;
    s32 c1 = 0;
    s32 c2 = 0;
    s32 lastFree = -1;
    for (s32 i = 0; i < 5; i++) {
        u8* slot = p + 0xA8 + i * 0x14;
        u8* obj = *(u8**)slot;
        if (obj == NULL) continue;
        fn_800B8E8C(obj, 0, *(s32*)(p + 0x10C + i * 0xC));
        fn_800B8E8C(*(u8**)slot, 1, *(s32*)(p + 0x110 + i * 0xC));
        fn_800B8E8C(*(u8**)slot, 2, *(s32*)(p + 0x114 + i * 0xC));
        s32 kind = -1;
        if (c0 < lbl_80346118[ClampedRayScene()] && lbl_80346158[ClampedRayScene()] > 0.0f &&
            lbl_803460E8[ClampedRayScene()] > 0.0f) {
            kind = 0;
            c0++;
        } else if (c1 < lbl_80346128[ClampedRayScene()] && lbl_80346178[ClampedRayScene()] > 0.0f &&
                   lbl_803460F8[ClampedRayScene()] > 0.0f) {
            kind = 1;
            c1++;
        } else if (c2 < lbl_80346138[ClampedRayScene()] && lbl_80346198[ClampedRayScene()] > 0.0f &&
                   lbl_80346108[ClampedRayScene()] > 0.0f) {
            kind = 2;
            c2++;
        } else {
            lastFree = i;
        }
        fn_800B9548(*(u8**)slot, kind);
        switch (fn_800B964C(*(u8**)slot)) {
        case 0:
            fn_800B9654(*(u8**)slot, 0.0f);
            fn_800B9994(*(u8**)slot, 1.0f);
            break;
        case 1:
            fn_800B9654(*(u8**)slot, 0.6f);
            fn_800B9994(*(u8**)slot, 1.0f);
            fn_800B999C(*(u8**)slot, 0, 1);
            break;
        case 2:
            fn_800B9654(*(u8**)slot, -0.6f);
            fn_800B9994(*(u8**)slot, 1.0f);
            fn_800B999C(*(u8**)slot, 0, 1);
            break;
        default:
            fn_800B991C(*(u8**)slot, 0);
            fn_800B999C(*(u8**)slot, 0, 1);
            break;
        }
        if (c1 == 0 && lastFree >= 0) {
            u8* other = *(u8**)(p + 0xA8 + lastFree * 0x14);
            fn_800B9548(other, 1);
            fn_800B991C(other, 0);
            fn_800B999C(other, 0, 1);
            fn_800B9994(other, 1.0f);
            c1++;
        }
        *(f32*)(slot + 0xC) = 0.0f;
        *(f32*)(slot + 0x8) = 0.0f;
        *(s32*)(slot + 0x4) = 0;
        *(f32*)(slot + 0x10) = 0.0f;
        fn_800B6328(*(u8**)(p + 0x94), slot, i);
    }
    for (s32 off = 0x160; off <= 0x1D4; off += 4) *(s32*)(p + off) = 0;
    *(f32*)(p + 0xA18) = 0.0f;
    *(f32*)(p + 0xA1C) = 0.0f;
    *(f32*)(p + 0xA20) = 0.0f;
}

// 0x800B9D94
// Walks all zone objects flagged 0x40 and files them by script hash into the owner's slot arrays
// (fighters, props, lists), tagging each with the owner id 0x2A75F102 or 0x83B4950A, setting the
// 0x400000 flag at +0x15C and calling the matching registration helper. Unknown kinds get their
// virtual slot 0x18 called under a scoped lock.
void fn_800B9D94(u8* p) {
    u8* a27 = p;
    u8* a26 = p;
    u8* a25 = p;
    u8* a24 = p;
    u8* a23 = p;
    u8* a22 = p;
    u8* a21 = p;
    u8* a20 = p;
    u8* a19 = p;
    u8* a18 = p;
    u8* zone0 = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    s32 total = *(s32*)(zone0 + 0x94) + *(s32*)(zone0 + 0x98);
    for (s32 i = 0; i < total; i++) {
        u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
        s32 nA = *(s32*)(zone + 0x94);
        u8* obj;
        u8* desc;
        if (i < nA) {
            obj = *(u8**)(*(u8**)(zone + 0x120) + i * 4);
            desc = *(u8**)(zone + 0x8C) + i * 0x40;
        } else {
            s32 k = i - nA;
            obj = *(u8**)(*(u8**)(zone + 0x12C) + k * 4);
            desc = *(u8**)(zone + 0x90) + (k << 6);
        }
        if (!(*(u32*)(desc + 0x1C) & 0x40) || obj == NULL) continue;
        u8* o = *(u8**)(obj + 0x9C);
        u32 tag = (o != NULL) ? *(u32*)(o + 8) : 0;
        switch (tag) {
        case 0xE4A6E770u:
        case 0xCF8BB4B3u:
        case 0xD69085F2u: {
            *(u32*)(obj + 0xF0) = 0x2A75F102u;
            *(u8**)(a27 + 0x94) = obj;
            *(u32*)(obj + 0x15C) = *(u32*)(obj + 0x15C) | 0x400000u;
            u8* o2 = *(u8**)(obj + 0x9C);
            u32 t2 = (o2 != NULL) ? *(u32*)(o2 + 8) : 0;
            if (t2 == 0xE4A6E770u) {
                u8* x = *(u8**)(a27 + 0x94);
                *(u8**)(a26 + 0xDC) = x;
                fn_800BCA44(x, 1);
                a26 += 4;
            } else {
                t2 = (o2 != NULL) ? *(u32*)(o2 + 8) : 0;
                if (t2 == 0xCF8BB4B3u) {
                    u8* x = *(u8**)(a27 + 0x94);
                    *(u8**)(a25 + 0x10C) = x;
                    fn_800BCA44(x, 2);
                    a25 += 4;
                } else {
                    t2 = (o2 != NULL) ? *(u32*)(o2 + 8) : 0;
                    if (t2 == 0xD69085F2u) {
                        u8* x = *(u8**)(a27 + 0x94);
                        *(u8**)(a24 + 0xF4) = x;
                        fn_800BCA44(x, 3);
                        a24 += 4;
                    }
                }
            }
            a27 += 4;
            break;
        }
        case 0xFD1C7B99u:
            *(u32*)(obj + 0xF0) = 0x83B4950Au;
            *(u32*)(obj + 0x15C) = *(u32*)(obj + 0x15C) | 0x400000u;
            fn_800BFBB0(p + 0x240, obj, 1);
            break;
        case 0xD631285Au:
            *(u32*)(obj + 0xF0) = 0x83B4950Au;
            *(u32*)(obj + 0x15C) = *(u32*)(obj + 0x15C) | 0x400000u;
            fn_800BFBB0(p + 0x240, obj, 2);
            break;
        case 0xCF2A191Bu:
            *(u32*)(obj + 0xF0) = 0x83B4950Au;
            *(u32*)(obj + 0x15C) = *(u32*)(obj + 0x15C) | 0x400000u;
            fn_800BFBB0(p + 0x240, obj, 3);
            break;
        case 0x806B8FDCu:
            *(u32*)(obj + 0xF0) = 0x83B4950Au;
            *(u32*)(obj + 0x15C) = *(u32*)(obj + 0x15C) | 0x400000u;
            fn_800BFBB0(p + 0x240, obj, 0);
            break;
        case 0x119F7A86u:
        case 0x47C5DD00u:
        case 0x3AB22945u:
        case 0x08844BC7u: {
            *(u32*)(obj + 0xF0) = 0x2A75F102u;
            *(u8**)(a23 + 0x130) = obj;
            *(u32*)(obj + 0x15C) = *(u32*)(obj + 0x15C) | 0x400000u;
            u8* o2 = *(u8**)(obj + 0x9C);
            u32 t2 = (o2 != NULL) ? *(u32*)(o2 + 8) : 0;
            if (t2 == 0x3AB22945u) {
                u8* x = *(u8**)(a23 + 0x130);
                *(u8**)(a22 + 0x160) = x;
                fn_800BD948(x, 0);
                a22 += 4;
            } else {
                t2 = (o2 != NULL) ? *(u32*)(o2 + 8) : 0;
                if (t2 == 0x119F7A86u) {
                    u8* x = *(u8**)(a23 + 0x130);
                    *(u8**)(a21 + 0x158) = x;
                    fn_800BD948(x, 1);
                    a21 += 4;
                } else {
                    t2 = (o2 != NULL) ? *(u32*)(o2 + 8) : 0;
                    if (t2 == 0x08844BC7u) {
                        u8* x = *(u8**)(a23 + 0x130);
                        *(u8**)(a20 + 0x150) = x;
                        fn_800BD948(x, 2);
                        a20 += 4;
                    } else {
                        t2 = (o2 != NULL) ? *(u32*)(o2 + 8) : 0;
                        if (t2 == 0x47C5DD00u) {
                            u8* x = *(u8**)(a23 + 0x130);
                            *(u8**)(a19 + 0x168) = x;
                            fn_800BD948(x, 3);
                            a19 += 4;
                        }
                    }
                }
            }
            a23 += 4;
            break;
        }
        case 0x7CA3BE1Cu:
            *(u32*)(obj + 0xF0) = 0x2A75F102u;
            *(u8**)(a18 + 0x170) = obj;
            *(u32*)(obj + 0x15C) = *(u32*)(obj + 0x15C) | 0x400000u;
            fn_800BE3A4(*(u8**)(a18 + 0x170));
            a18 += 4;
            break;
        case 0x9E8C9E77u:
            *(u32*)(obj + 0xF0) = 0x83B4950Au;
            *(u8**)(p + 0x238) = obj;
            *(u32*)(obj + 0x15C) = *(u32*)(obj + 0x15C) | 0x400000u;
            fn_800BF6FC(*(u8**)(p + 0x238));
            break;
        case 0xE59ED039u:
            *(u32*)(obj + 0xF0) = 0x83B4950Au;
            *(u8**)(p + 0x23C) = obj;
            *(u32*)(obj + 0x15C) = *(u32*)(obj + 0x15C) | 0x400000u;
            break;
        case 0xD88D8126u:
            *(u32*)(obj + 0xF0) = 0x83B4950Au;
            break;
        default: {
            ScopedLock lock;
            ScopedLockCtor(&lock);
            (*(VFnInt*)(*(u8**)obj + 0x18))(obj, 1);
            ScopedLockDtor(&lock, -1);
            break;
        }
        }
    }
}

// 0x800CF7E8
// Sets up the prop's sub-objects: seeds the rng-driven value at +0x2FFC, then for each zone
// object of the known script hashes registers it with fn_800C8E54 (kind 0..0x16) or stores it at
// +0xA4 / +0xA8, and finally aims the camera object at +0x30F4 and refreshes dependent values.
void fn_800CF7E8(u8* p) {
    f32 vec[3];
    fn_800CD490(p + 0x2FFC, RaySceneTest(p));
    u8* world = *(u8**)gWorldState;
    u8* zone = *(u8**)(world + 0x1A4) + gZoneResourceSize;
    for (s32 n = 0; n < *(s32*)(*(u8**)gWorldState + 4); n++) {
        for (s32 j = 0; j < *(s32*)(zone + 0x98); j++) {
            u8* obj = *(u8**)(*(u8**)(zone + 0x12C) + j * 4);
            if (*(u8**)(obj + 0x9C) == NULL) continue;
            u8* o = *(u8**)(obj + 0x9C);
            u32 tag = (o != NULL) ? *(u32*)(o + 8) : 0;
            switch (tag) {
            case 0x59B9FFDBu: fn_800C8E54(obj, p + 0xF4, 0); break;
            case 0xBB2F4E0Bu: fn_800C8E54(obj, p + 0xF4, 1); break;
            case 0x90021DC8u: fn_800C8E54(obj, p + 0xF4, 2); break;
            case 0x89192C89u: fn_800C8E54(obj, p + 0xF4, 3); break;
            case 0xC658BA4Eu: fn_800C8E54(obj, p + 0xF4, 4); break;
            case 0xDF438B0Fu: fn_800C8E54(obj, p + 0xF4, 5); break;
            case 0xED75E98Du: fn_800C8E54(obj, p + 0xF4, 7); break;
            case 0x73F6C403u: fn_800C8E54(obj, p + 0xF4, 9); break;
            case 0xA3F6157Du: fn_800C8E54(obj, p + 0xF4, 0xA); break;
            case 0x91C077FFu: fn_800C8E54(obj, p + 0xF4, 0xC); break;
            case 0x88DB46BEu: fn_800C8E54(obj, p + 0xF4, 0xD); break;
            case 0xDE81E138u: fn_800C8E54(obj, p + 0xF4, 0xF); break;
            case 0xF5ACB2FBu: fn_800C8E54(obj, p + 0xF4, 0x10); break;
            case 0xECB783BAu: fn_800C8E54(obj, p + 0xF4, 0x11); break;
            case 0x6B2F9F75u: fn_800C8E54(obj, p + 0xF4, 0x12); break;
            case 0x7234AE34u: fn_800C8E54(obj, p + 0xF4, 0x13); break;
            case 0xA1B0AB24u: fn_800C8E54(obj, p + 0xF4, 0x14); break;
            case 0xB8AB9A65u: fn_800C8E54(obj, p + 0xF4, 0x15); break;
            case 0x9386C9A6u: fn_800C8E54(obj, p + 0xF4, 0x16); break;
            case 0x3A8BE3CBu: fn_800C8E54(obj, p + 0xF4, 0); break;
            case 0x9B50E644u:
                *(u8**)(p + 0xA4) = obj;
                *(u32*)(obj + 0x15C) = *(u32*)(obj + 0x15C) | 0x400000u;
                break;
            case 0x7E1D3A03u:
                *(u32*)(obj + 0x15C) = *(u32*)(obj + 0x15C) | 0x400000u;
                fn_800CDC9C(p + 0xA8);
                break;
            default:
                break;
            }
        }
    }
    (*(VFnSelfArg*)(*(u8**)(p + 0x30F4) + 0xC))(p + 0x3020, p + 0x2FFC);
    fn_800C75E8(p + 0xF4, p + 0x3020);
    (*(VFnSelf*)(*(u8**)(p + 0x30F4) + 0x20))(p + 0x3020);
    (*(VFnGetVec*)(*(u8**)(p + 0x30F4) + 0x24))(vec, p + 0x3020);
    fn_800CD9F0(p + 0xA8, vec);
    fn_800CC854(*(u8**)(p + 0xA4), p + 0xF4);
}

// 0x800D5174
// Path follower step: moves the point at s+0x18 toward the current waypoint by speed*dt. When the
// step overshoots the waypoint it snaps to it, advances to the next waypoint (fn_800D6A24) and
// continues with the remaining distance; if the path ends (+0x14 set) it snaps to that end point.
void fn_800D5174(u8** self, f32 dt) {
    u8* s = *self;
    if (*(u8**)(s + 0x14) != NULL) return;
    if (*(s32*)(s + 0x8) != 0) return;
    f32 dist = *(f32*)(s + 0x0) * dt;
    f32 dist2 = dist * dist;
    u8* tgt = PathTarget(s);
    Vec3 d = { ((f32*)tgt)[0] - *(f32*)(s + 0x18), ((f32*)tgt)[1] - *(f32*)(s + 0x1C),
               ((f32*)tgt)[2] - *(f32*)(s + 0x20) };
    f32 len2 = d.z * d.z + (d.x * d.x + d.y * d.y);
    Vec3 dir = Vec3Normalized(d);
    *(f32*)(s + 0x18) = *(f32*)(s + 0x18) + dir.x * dist;
    *(f32*)(s + 0x1C) = *(f32*)(s + 0x1C) + dir.y * dist;
    *(f32*)(s + 0x20) = *(f32*)(s + 0x20) + dir.z * dist;
    if (dist2 <= len2) return;
    while (dist2 > len2) {
        s = *self;
        tgt = PathTarget(s);
        *(f32*)(s + 0x18) = ((f32*)tgt)[0];
        *(f32*)(s + 0x1C) = ((f32*)tgt)[1];
        *(f32*)(s + 0x20) = ((f32*)tgt)[2];
        f32 len = (len2 <= 0.0f) ? 0.0f : sqrtf(len2);
        dist -= len;
        dist2 = dist * dist;
        fn_800D6A24(self);
        s = *self;
        u8* end = *(u8**)(s + 0x14);
        if (end != NULL) {
            *(f32*)(s + 0x18) = *(f32*)(end + 0x34);
            *(f32*)(s + 0x1C) = *(f32*)(end + 0x38);
            *(f32*)(s + 0x20) = *(f32*)(end + 0x3C);
            return;
        }
        tgt = PathTarget(s);
        d.x = ((f32*)tgt)[0] - *(f32*)(s + 0x18);
        d.y = ((f32*)tgt)[1] - *(f32*)(s + 0x1C);
        d.z = ((f32*)tgt)[2] - *(f32*)(s + 0x20);
        len2 = d.z * d.z + (d.x * d.x + d.y * d.y);
    }
    dir = Vec3Normalized(d);
    s = *self;
    *(f32*)(s + 0x18) = *(f32*)(s + 0x18) + dir.x * dist;
    *(f32*)(s + 0x1C) = *(f32*)(s + 0x1C) + dir.y * dist;
    *(f32*)(s + 0x20) = *(f32*)(s + 0x20) + dir.z * dist;
}

// 0x801F9D3C
// Init: runs fn_801FA7CC, sets the vtable and some scalars, then builds an orthonormal basis
// around the +Z axis stored at +0xA8 (forward at +0xA8, right/up vectors at +0xB4 and +0xC0).
u8* fn_801F9D3C(u8* p, s32 a4) {
    fn_801FA7CC(p, a4);
    *(void**)p = lbl_80386428;
    *(s32*)(p + 0xCC) = 0;
    *(s32*)(p + 0xD4) = -1;
    *(f32*)(p + 0xE8) = 0.0f;
    *(s32*)(p + 0xEC) = 0;
    *(s32*)(p + 0xF0) = 3;
    *(s32*)(p + 0xF4) = 0;
    *(f32*)(p + 0xF8) = 1.0f;
    *(f32*)(p + 0xFC) = 0.0f;
    Vec3 axis = Vec3Normalized(Vec3{ 0.0f, 0.0f, 1.0f });
    *(f32*)(p + 0xA8) = axis.x;
    *(f32*)(p + 0xAC) = axis.y;
    *(f32*)(p + 0xB0) = axis.z;
    if (axis.x == 0.0f && axis.y == 0.0f && axis.z == 1.0f) {
        *(f32*)(p + 0xB4) = 0.0f;
        *(f32*)(p + 0xB8) = 1.0f;
        *(f32*)(p + 0xBC) = 0.0f;
        *(f32*)(p + 0xC0) = -1.0f;
        *(f32*)(p + 0xC4) = 0.0f;
        *(f32*)(p + 0xC8) = 0.0f;
    } else {
        // Project (1,1,1) off the axis, normalise, then cross with the axis.
        f32 s = 1.0f * axis.x + 1.0f * axis.y + 1.0f * axis.z;
        Vec3 v = Vec3Normalized(Vec3{ 1.0f - axis.x * s, 1.0f - axis.y * s, 1.0f - axis.z * s });
        *(f32*)(p + 0xB4) = v.x;
        *(f32*)(p + 0xB8) = v.y;
        *(f32*)(p + 0xBC) = v.z;
        *(f32*)(p + 0xC0) = axis.y * v.z - axis.z * v.y;
        *(f32*)(p + 0xC4) = axis.z * v.x - axis.x * v.z;
        *(f32*)(p + 0xC8) = axis.x * v.y - axis.y * v.x;
    }
    *(f32*)(p + 0xD8) = 0.0f;
    *(f32*)(p + 0xDC) = 0.0f;
    *(f32*)(p + 0xE0) = 0.0f;
    *(f32*)(p + 0xE4) = 0.0f;
    return p;
}

}  // extern "C"
