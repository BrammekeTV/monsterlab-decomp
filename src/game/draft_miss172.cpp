// Drafted functions: save reset, lab-select UI refresh, and assorted 0x80xxxxxx helpers.
#include <string.h>

#include "types.h"
#include "engine/math.h"
#include "engine/rend.h"
#include "game/part.h"

typedef f32 Scalar;

extern "C" {
#include "game/script_callbacks.h"
}

struct StringTable;
extern "C++" void* gScreenSets;               // 0x805FB654
extern "C++" StringTable* gStringTable;       // 0x805FB910
extern "C++" u8 gFlag805FB948;                // 0x805FB948 (two-player mode?)
extern "C++" u8* fn_801B55A8(u8* ent);        // 0x801B55A8 (C++ linkage; returns the locator count as an integer value)

extern "C" {
void* CharacterFind(s32 id);                                          // 0x800E876C
void EntitySyncTransform(void* entity, int a);                        // 0x801B4F4C
void LabInventoryReset(u8* inv);                                      // lab_inventory.cpp
u32 RandomU32(void);                                                  // 0x80143460
void* ScreenSetFindWidget(void* set, u32 hash);                       // 0x80198E34
void* SpawnHelper(void* owner, void* target);                         // 0x8000C478
int SysSprintf(char* buf, const char* fmt, ...);
void UISetText(void* el, const char* text, s32 a);                    // 0x8019EFC8
void* UIWidgetFindChild(void* widget, u32 hash);                      // 0x80199AEC
f32 Vec3GetX(const Vec3* v);
f32 Vec3GetY(const Vec3* v);
f32 Vec3GetZ(const Vec3* v);
void Vec3MulScalar(Vec3* out, const Vec3* v, const Scalar* s);        // 0x80053224
void Vec3NormalizeTo(Vec3* out, const Vec3* v);                       // 0x80055D9C
void Vec3SetX(Vec3* v, f32 x);
void Vec3SetY(Vec3* v, f32 y);
void Vec3SetZ(Vec3* v, f32 z);
void Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);                // 0x80053370
void ScalarSet(Scalar* out, f32 v);

u8* fn_8004B498(u8* p);                                               // 0x8004B498
void fn_8004B604(u8* p);                                              // 0x8004B604
u8* fn_8004E684(u8* p, s32 a, void* b, s32 c);                        // 0x8004E684
u8* fn_80052530(u8* p, s32 a, void* b, s32 c);                        // 0x80052530
void fn_8005263C(f32* q);                                             // 0x8005263C
u8* fn_800526D4(u8* a3);                                              // 0x800526D4
u8* fn_800526DC(u8* a3);                                             // 0x800526DC
f32 fn_80053208(u8* a3);                                              // 0x80053208
u32 fn_800533A4(u8* a3);                                              // 0x800533A4
void fn_80054484(Vec3* out, const Vec3* a, const Vec3* b);            // 0x80054484
void fn_80054530(f32* out, f32* a, f32* b);                           // 0x80054530
void fn_8005484C(f32* q);                                             // 0x8005484C
void fn_80054AC0(u8* obj, const Vec3* v);                             // 0x80054AC0
void fn_80055828(f32* out, u8* mtx, const Vec3* v);                   // 0x80055828
void fn_800557B4(u8* out);                                            // 0x800557B4
void fn_800359F4(u8* a3, f32 f1, f32 f2, f32 f3);                     // 0x800359F4
u8* fn_8005CA88(u8* p);                                               // 0x8005CA88
u8* fn_8005CC30(u8* p, s32 a, void* b, s32 c);                        // 0x8005CC30
u8* fn_8006307C(u8* p);                                               // 0x8006307C
u8* fn_800636BC(u8* p);                                               // 0x800636BC
s32 fn_80064568(void* fxMgr, u32 hash, const f32* v, u8** out);      // 0x80064568
s32 fn_80065200(u8* arg0, u8* arg1, void* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6); // 0x80065200
void fn_800690D4(u8* p, s32 a, void* params, s32 b);                  // 0x800690D4
u8* fn_8006CCC4(u8* obj, s32 a1, void* a2, s32 a3);                   // 0x8006CCC4
u8* fn_8006CFA4(u8* obj, s32 a, void* params, s32 b);                 // 0x8006CFA4
u8* fn_8006D2D0(u8* p);                        // 0x8006D2D0
u8* fn_8006F61C(u8* arg0);                                            // 0x8006F61C
u8* fn_8006F6BC(u8* p);                        // 0x8006F6BC
void fn_800782AC(u8* obj, s32* outB, s32* outA);                      // 0x800782AC
s32 fn_8007C5FC(u8* rec, s32 a, s32 b, s32 c, u8* d, u8* e);          // 0x8007C5FC
void fn_8007D2BC(u8* a3, f32 f1);                                     // 0x8007D2BC
f32 fn_8007DD7C(Vec3* v);                                             // 0x8007DD7C
u8* fn_80081A34(void);                                                // 0x80081A34
f32 fn_80082114(u8* a3);                                              // 0x80082114
void fn_80082120(u8* obj, f32* v);                                    // 0x80082120
void fn_80081E24(u8* obj, f32* v);                                    // 0x80081E24
s32 fn_8008294C(u8* a3);                                              // 0x8008294C
void fn_80082954(Vec3* out, u8* obj, f32 t);                          // 0x80082954
f32 fn_80082A04(u8* a3);                                              // 0x80082A04
u32 fn_80082A0C(u8* a3, s32 a4);                                      // 0x80082A0C
void fn_80082A20(u8* a3, f32 f1);                                     // 0x80082A20
void fn_80082A2C(u8* a3, f32 f1);                                     // 0x80082A2C
void fn_80082A38(u8* a3, f32 f1);                                     // 0x80082A38
void fn_80082A44(u8* a3, f32 f1);                                     // 0x80082A44
f32 fn_80082A50(u8* a3, u8* a4);                                      // 0x80082A50
void fn_80082A74(f32* v, f32* s);                                     // 0x80082A74
void fn_80082AC4(f32* a, f32* b);                                     // 0x80082AC4
void fn_80082B20(u8* self, f32* src);                                 // 0x80082B20
s32 fn_80082B88(u8* a3);                                              // 0x80082B88
void fn_80082B90(u8* obj, s32 idx);                                   // 0x80082B90
f32 fn_80082BE0(u8* a3, u8* a4);                                      // 0x80082BE0
void fn_80082C04(u8* obj, f32* v);                                    // 0x80082C04
void fn_80082D88(u8* obj, f32* v);                                    // 0x80082D88
void fn_80082F0C(u8* a3, f32 f1);                                     // 0x80082F0C
f32 fn_80082F20(u8* a3, u8* a4);                                      // 0x80082F20
u8* fn_80082F3C(u8* a3);                                              // 0x80082F3C
f32 fn_80082F44(u8* a3, u8* a4);                                      // 0x80082F44
void fn_80082F68(f32* out, f32* in);                                  // 0x80082F68
s32 fn_80082F9C(u8* obj, u8* arg);                                    // 0x80082F9C
s32 fn_80083314(u8* obj, s32 arg);                                    // 0x80083314
void fn_80084D3C(f32* out, u8* ctx);                                  // 0x80084D3C
void* fn_801088C0(void* a0, u32 a1, void* a2);                        // 0x801088C0
u8* fn_801094F4(char* name, s32 flags);                               // 0x801094F4
void fn_80142ED0(u8* base, s32 size);                                 // 0x80142ED0
void fn_80144624(u8* mtx, const Vec3* axis, f32 angle);               // 0x80144624
void fn_8014DE68(void);                                               // 0x8014DE68
void fn_80154398(void);                                               // 0x80154398
s32 fn_8015BA30(void* dl);                                            // 0x8015BA30
s32 fn_8016A324(u8* p);                                              // 0x8016A324
s32 fn_80173568(u8* p);                                              // 0x80173568
s32 fn_80175088(u8* p);                                              // 0x80175088
u32 fn_8018409C(u8* p);                                               // 0x8018409C
void* fn_801BC574(void* p);                                           // 0x801BC574
u8* fn_801B4334(u8* arg0, s32 layer, u8* ent, s32 reset);                                              // 0x801B4334
void fn_801B55F4(u8* obj, u8* out, s32 idx);                          // 0x801B55F4
void fn_801B6F34(u8* p);                                              // 0x801B6F34
void fn_801D94CC(u8* p);                                              // 0x801D94CC
void fn_801D9A24(u8* p);                                              // 0x801D9A24
void fn_801DA1CC(u8* p);                                              // 0x801DA1CC
void fn_801DA7C8(u8* p);                                              // 0x801DA7C8
void fn_801DA81C(u8* p);                                              // 0x801DA81C
void fn_801DB160(u8* p);                                              // 0x801DB160
void fn_801EC080(u8* a, u8* b);                                       // 0x801EC080
u32 fn_801EC880(u8* p, u32 hash);                                     // 0x801EC880
void fn_8012F6B0(u8* p);                                              // 0x8012F6B0
s32 fn_802FA554(u8* rec);                                             // 0x802FA554
s32 fn_802FABA4(u8* rec);                                             // 0x802FABA4

extern void* gStringTable2;                  // 0x805FB90C
extern u8 lbl_80348950[];                    // 0x80348950
extern u8 lbl_80348A10[];                    // 0x80348A10
extern u32 lbl_80348AD0;                     // 0x80348AD0
extern u8 lbl_80375C48[];                    // 0x80375C48
extern u8 lbl_80400FF0[];                    // 0x80400FF0
extern u8 lbl_80401108[];                    // 0x80401108 (display-list ring, 0x94 bytes each)
extern u8 lbl_8047C760[];                    // 0x8047C760
extern u8 lbl_80556E80[];                    // 0x80556E80
extern u8 lbl_803C4298[];                    // 0x803C4298
extern u32 lbl_803F0E30[];                   // 0x803F0E30
extern u32 lbl_803F0F30[];                   // 0x803F0F30
extern u32 lbl_8040ADF0;                     // 0x8040ADF0
extern s32 lbl_8040AE10;                     // 0x8040AE10
extern s32 lbl_805FB35C;                     // 0x805FB35C
extern s32 lbl_805FB4B0;                     // 0x805FB4B0
extern s32 lbl_805FB4BC;                     // 0x805FB4BC (DL being recorded)
extern s32 lbl_805FB4C0;                     // 0x805FB4C0
extern s32 lbl_805FB4C4;                     // 0x805FB4C4
extern s32 lbl_805FB4C8;                     // 0x805FB4C8
extern s32 lbl_805FB4CC;                     // 0x805FB4CC
extern u8* lbl_805FB4D0;                     // 0x805FB4D0
extern s32 lbl_805FB4D4;                     // 0x805FB4D4
extern s32 lbl_805FB4D8;                     // 0x805FB4D8
extern s32 lbl_805FB4DC;                     // 0x805FB4DC
extern s32 lbl_805FB4E0;                     // 0x805FB4E0
extern s32 lbl_805FB4E8;                     // 0x805FB4E8
extern s32 lbl_805FB4EC;                     // 0x805FB4EC
extern u8 lbl_805FB50C;                      // 0x805FB50C (GX shadow flags byte)
extern u8* lbl_805FBBE0;                     // 0x805FBBE0 (save block pointer)
extern f32 lbl_805FD6A8;                     // 0x805FD6A8
extern f32 lbl_805FD800;                     // 0x805FD800 (0.0f)
extern f32 lbl_805FD808;                     // 0x805FD808
extern f32 lbl_805FD824;                     // 0x805FD824
extern f32 lbl_805FD82C;                     // 0x805FD82C
extern f32 lbl_805FD830;                     // 0x805FD830
extern f32 lbl_805FD834;                     // 0x805FD834
extern f32 lbl_805FEB04;                     // 0x805FEB04 (1.0f)
extern f32 lbl_805FEB0C;                     // 0x805FEB0C (0.0f)
extern f32 lbl_805FEB1C;                     // 0x805FEB1C
extern f32 lbl_805FEB50;                     // 0x805FEB50
extern f32 lbl_805FEB54;                     // 0x805FEB54
extern f32 lbl_805FEB58;                     // 0x805FEB58
extern f32 lbl_805FEB5C;                     // 0x805FEB5C
extern const char lbl_8035ABC0[];            // 0x8035ABC0 (format)
extern const char lbl_80354800[];            // 0x80354800
extern const char lbl_8035480C[];            // 0x8035480C
extern const char lbl_8036D060[];            // 0x8036D060 (format strings)
extern const char lbl_80376850[];            // 0x80376850 (format strings)
extern const char* lbl_805F8F10;             // 0x805F8F10
extern const char lbl_805F8F14[];            // 0x805F8F14 ("%d")
extern const char lbl_805F8200[];            // 0x805F8200 (empty string)
extern const char lbl_805F9068[];            // 0x805F9068
extern const char lbl_805F9260[];            // 0x805F9260
extern const char lbl_805F9264[];            // 0x805F9264
extern const char lbl_805F926C[];            // 0x805F926C
extern const char lbl_805F9274[];            // 0x805F9274
extern const char lbl_805F927C[];            // 0x805F927C
extern const char lbl_805F9284[];            // 0x805F9284
extern const char lbl_805F9290[];            // 0x805F9290
extern const char lbl_805F9298[];            // 0x805F9298
extern const char lbl_805F92A0[];            // 0x805F92A0
extern const char lbl_805F92A8[];            // 0x805F92A8
extern const char lbl_805F92B0[];            // 0x805F92B0
extern const char lbl_805F92B8[];            // 0x805F92B8
extern const char lbl_805F92C0[];            // 0x805F92C0
extern const char lbl_805F92C8[];            // 0x805F92C8
extern const char lbl_805F92D0[];            // 0x805F92D0
extern const char lbl_805F92D8[];            // 0x805F92D8
extern const char lbl_805F92E0[];            // 0x805F92E0
extern const char lbl_805F92E8[];            // 0x805F92E8
extern const char lbl_805F92F0[];            // 0x805F92F0
extern const char lbl_805F92F4[];            // 0x805F92F4
extern const char lbl_805F9300[];            // 0x805F9300
extern const char lbl_805F9308[];            // 0x805F9308
extern const char lbl_805F9310[];            // 0x805F9310
extern const char lbl_805F9318[];            // 0x805F9318

// Write-gather pipe helpers: BP register write (0x61 + word) and XF register write.
static void GxBpWrite(u32 value) {
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = value;
}

static void GxXfWrite(u16 addr, u32 value) {
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = addr;
    GXWGFifo._u32 = value;
}

// 0x8013AFF0
// Refreshes the four lab-select slot widgets: for each slot whose dirty flag is set, clears it
// and shows/hides the child widgets and texts according to the slot's save state.
void fn_8013AFF0(void) {
    typedef void (*WidgetFn)(void*, s32);
    u8* set = *(u8**)((u8*)gScreenSets + 0x88);
    if (set == NULL) return;
    u8* page = (u8*)ScreenSetFindWidget(set, 0x9DE0B3FEu);
    if (page == NULL) return;
    for (s32 i = 0; i < 4; i++) {
        s32 slot = (s32)lbl_803F0E30[lbl_805FB35C + i];
        u32 flags = lbl_803F0F30[slot];
        if (!(flags & 1)) continue;
        lbl_803F0F30[slot] = flags & ~1u;
        u8* w0 = (u8*)UIWidgetFindChild(page, *(u32*)(lbl_80375C48 + 0x48 + i * 4));
        u8* w1 = (u8*)UIWidgetFindChild(page, *(u32*)(lbl_80375C48 + 0x58 + i * 4));
        u8* w2 = (u8*)UIWidgetFindChild(page, *(u32*)(lbl_80375C48 + 0x68 + i * 4));
        u8* w3 = (u8*)UIWidgetFindChild(page, *(u32*)(lbl_80375C48 + 0x78 + i * 4));
        u8* w4 = (u8*)UIWidgetFindChild(page, *(u32*)(lbl_80375C48 + 0x88 + i * 4));
        u8* w5 = (u8*)UIWidgetFindChild(page, *(u32*)(lbl_80375C48 + 0xB8 + i * 4));
        u8* w6 = (u8*)UIWidgetFindChild(page, *(u32*)(lbl_80375C48 + 0xC8 + i * 4));
        u8* w7 = (u8*)UIWidgetFindChild(page, *(u32*)(lbl_80375C48 + 0x98 + i * 4));
        u8* w8 = (u8*)UIWidgetFindChild(page, *(u32*)(lbl_80375C48 + 0xA8 + i * 4));
        if (!w0 || !w1 || !w2 || !w3 || !w4 || !w5 || !w6 || !w7 || !w8) continue;
        u8* rec = lbl_80556E80 + slot * 0xC + 0x40;
        if (fn_802FABA4(rec) == 0) {
            (*(WidgetFn*)(*(u8**)(w3 + 0x138) + 0x30))(w3, 2);
            (*(WidgetFn*)(*(u8**)(w2 + 0x138) + 0x2C))(w2, 2);
            continue;
        }
        (*(WidgetFn*)(*(u8**)(w5 + 0x138) + 0x30))(w5, 2);
        (*(WidgetFn*)(*(u8**)(w4 + 0x138) + 0x30))(w4, 2);
        (*(WidgetFn*)(*(u8**)(w6 + 0x138) + 0x30))(w6, 2);
        (*(WidgetFn*)(*(u8**)(w8 + 0x138) + 0x30))(w8, 2);
        if (fn_802FA554(rec) == 0) {
            UISetText(w0, (const char*)(lbl_80556E80 + slot * 0x20 + 0x340), 1);
            UISetText(w1, lbl_805F9068, 1);
            u8* st = (u8*)gStringTable;
            const char* str;
            if (*(u8**)st != NULL && *(u32*)(st + 0xC) > 0x68Au) {
                str = *(const char**)(*(u8**)st + 0x1A28);
            } else {
                str = lbl_805F8200;
            }
            UISetText(w7, str, 1);
            (*(WidgetFn*)(*(u8**)(w8 + 0x138) + 0x2C))(w8, 2);
        } else {
            UISetText(w0, lbl_805F9068, 1);
            UISetText(w1, (const char*)(lbl_80556E80 + slot * 0x20 + 0x340), 1);
            UISetText(w7, lbl_805F9068, 1);
            u32 f = lbl_803F0F30[slot];
            if (f & 2) {
                (*(WidgetFn*)(*(u8**)(w4 + 0x138) + 0x2C))(w4, 2);
            } else if (f & 0x10) {
                (*(WidgetFn*)(*(u8**)(w5 + 0x138) + 0x2C))(w5, 2);
            } else if ((f & 4) || (f & 8)) {
                (*(WidgetFn*)(*(u8**)(w6 + 0x138) + 0x2C))(w6, 2);
            }
        }
        (*(WidgetFn*)(*(u8**)(w2 + 0x138) + 0x30))(w2, 2);
        (*(WidgetFn*)(*(u8**)(w3 + 0x138) + 0x2C))(w3, 2);
    }
}

// 0x80222300
// Resets a save block to its new-game defaults (counters zeroed, id slots set to -1).
void SaveReset(u8* s) {
    *(s32*)(s + 0xFC) = 0;
    *(s32*)(s + 0xF8) = 0;
    *(s32*)(s + 0xF4) = 0;
    *(s32*)(s + 0xF0) = 0;
    *(s32*)(s + 0xE4) = 0;
    *(s32*)(s + 0xE0) = 0;
    *(s32*)(s + 0xEC) = 0x80001;
    *(s32*)(s + 0xE8) = 0;
    *(s32*)(s + 0x104) = -1;
    *(s32*)(s + 0x108) = -1;
    *(s32*)(s + 0x10C) = -1;
    *(s32*)(s + 0x6AC) = -1;
    *(s32*)(s + 0x6A8) = 0;
    *(s32*)(s + 0x1C) = 0;
    *(s32*)(s + 0x18) = 0;
    *(s32*)(s + 0x14) = 0;
    *(s32*)(s + 0x10) = 0;
    *(s32*)(s + 0x120) = 0;
    *(s32*)(s + 0x11C) = 0;
    *(u8*)(s + 0x124) = 0;
    *(s32*)(s + 0x3C) = 0;
    *(s32*)(s + 0x38) = 0;
    *(s32*)(s + 0x34) = 0;
    *(s32*)(s + 0x30) = 0;
    *(s32*)(s + 0x2C) = 0;
    *(s32*)(s + 0x28) = 0;
    *(s32*)(s + 0x24) = 0;
    *(s32*)(s + 0x20) = 0;
    *(s32*)(s + 0x128) = 0;
    *(s32*)(s + 0x12C) = 0;
    *(s32*)(s + 0x8EC) = 0;
    *(s32*)(s + 0x8F0) = 0;
    *(s32*)(s + 0x8F4) = 0;
    *(s32*)(s + 0x8F8) = 0;
    *(s32*)(s + 0x908) = 0;
    *(s32*)(s + 0x90C) = 0;
    *(s32*)(s + 0x134) = 0;
    *(s32*)(s + 0x904) = 0;
    *(s32*)(s + 0x900) = 0;
    *(s32*)(s + 0x144) = 0;
    *(s32*)(s + 0x140) = 0;
    *(s32*)(s + 0x118) = 0;
    *(s32*)(s + 0x138) = -1;
    *(s32*)(s + 0x148) = 0;
    *(u8*)(s + 0xA24) = 0;
    *(s32*)(s + 0x110) = 0;
    *(s32*)(s + 0x6B8) = 0;
    *(s32*)(s + 0x6B4) = 0;
    *(s32*)(s + 0x114) = 0;
    *(s32*)(s + 0x910) = 1;
    *(s32*)(s + 0xA20) = 0;
    *(s32*)(s + 0x8E8) = 0;
    // 0x9C bytes of byte flags at +0x40
    for (s32 i = 0; i < 0x9C; i++) *(u8*)(s + 0x40 + i) = 0;
    // 47 records of 6 words at +0x150, stride 0x18
    for (s32 i = 0; i < 0x2F; i++) {
        u8* r = s + i * 0x18;
        *(s32*)(r + 0x160) = 0;
        *(s32*)(r + 0x15C) = 0;
        *(s32*)(r + 0x158) = 0;
        *(s32*)(r + 0x154) = 0;
        *(s32*)(r + 0x150) = 0;
        *(s32*)(r + 0x164) = 0;
    }
    for (s32 o = 0x5B8; o <= 0x6A4; o += 4) *(s32*)(s + o) = 0;
    for (s32 o = 0x6BC; o <= 0x738; o += 4) *(s32*)(s + o) = -1;
    LabInventoryReset(s + 0xA30);
    *(s32*)(s + 0x928) = 0;
    *(s32*)(s + 0x92C) = 0;
    *(s32*)(s + 0x930) = 0;
    *(s32*)(s + 0x934) = 0;
    *(s32*)(s + 0x938) = 0;
    *(s32*)(s + 0x93C) = 0;
    *(s32*)(s + 0x940) = 0;
    *(s32*)(s + 0x944) = 0;
    *(s32*)(s + 0x948) = 0;
    *(s32*)(s + 0x94C) = 0;
    *(s32*)(s + 0x950) = 0;
    *(s32*)(s + 0x954) = 0;
    *(s32*)(s + 0x958) = 0;
    *(s32*)(s + 0x980) = 0;
    *(s32*)(s + 0x9A8) = 0;
    *(s32*)(s + 0x95C) = 0;
    *(s32*)(s + 0x984) = 0;
    *(s32*)(s + 0x9AC) = 0;
    for (s32 o = 0x960; o <= 0x97C; o += 4) {
        *(s32*)(s + o) = 0;
        *(s32*)(s + o + 0x28) = 0;
        *(s32*)(s + o + 0x50) = 0;
    }
    for (s32 o = 0xA04; o <= 0xA18; o += 4) *(s32*)(s + o) = 0;
    for (s32 o = 0x9D0; o <= 0xA00; o += 4) *(s32*)(s + o) = 0;
    *(s32*)(s + 0xA1C) = 0;
    *(u8*)(s + 0xA25) = 1;
}

// 0x800446BC
// Switches the GX state for drawing into a render target (arg1 = target descriptor) or, with
// arg1 NULL, restores the normal state; emits BP/XF writes to the write-gather pipe.
void fn_800446BC(u8* arg0, u8* arg1) {
    if (arg1 != NULL) {
        if (lbl_805FB4C0 != 0 &&
            fn_8015BA30(lbl_80401108 + lbl_805FB4BC * 0x94) + 0x10C0 > lbl_805FB4C8) {
            fn_8014DE68();
        }
        if (!(lbl_805FB50C & 2)) {
            u8 fl = lbl_805FB50C | 2;
            lbl_805FB50C = fl;
            if (fl & 1) {
                GxBpWrite(0xFE00000F);
                GxBpWrite(1);
                GxXfWrite(0x103F, 1);
                GxBpWrite(0xFE003C00);
                GxBpWrite(0x400);
                GxXfWrite(0x1010, 0x441);
                GxBpWrite(0x283C0000);
                GxBpWrite(0xC008F2A4);
                GxBpWrite(0xC108E6F0);
                GxBpWrite(0xC208F80F);
                GxBpWrite(0xC308F070);
            } else {
                GxBpWrite(0xFE00000F);
                GxBpWrite(1);
                GxXfWrite(0x103F, 1);
                GxBpWrite(0xFE003C00);
                GxBpWrite(0);
                GxXfWrite(0x100E, 0x442);
                GxXfWrite(0x1010, 0x442);
                GxBpWrite(0x28380040);
                GxBpWrite(0xC008F8AF);
                GxBpWrite(0xC108F2F0);
            }
        }
        GxBpWrite(0xFE000060);
        GxBpWrite((u32)*(u8*)(arg1 + 0x16) << 5 | 0x80000000);
        u32 w1c = *(u32*)(arg1 + 0x1C);
        if (lbl_8040ADF0 != w1c || lbl_8040AE10 != 0) {
            lbl_805FB4B0 += 1;
            lbl_8040ADF0 = *(u32*)(arg1 + 0x1C);
            u32 hw = lbl_80348AD0;
            lbl_8040AE10 = 0;
            GxBpWrite((((u32)*(u8*)(arg1 + 0x13) - 1) & 0xF) << 12 | 0x84000000);
            u32 fmt = *(u8*)(arg1 + 0x17);
            u32 v88 = (u32)*(u8*)(arg1 + 0x14) << 20 | 0x88000000 |
                      (((u32)*(u16*)(arg1 + 0xA) - 1) << 10) | ((u32)*(u16*)(arg1 + 0x8) - 1);
            GxBpWrite(v88);
            GxBpWrite((*(u32*)(lbl_80348950 + fmt * 0x20) >> 5) | 0x8C0D0000 | 0x8000);
            s32 alt = *(s32*)(lbl_80348A10 + fmt * 0x20);
            if (alt >= 0) {
                GxBpWrite(((u32)alt >> 5) | 0x900D0000 | 0x8000);
            } else {
                GxBpWrite(0x90000000);
            }
            GxBpWrite(((*(u32*)(arg1 + 0x1C) >> 5) & 0xFFFFFF) | 0x94000000);
            if (*(u8*)(arg1 + 0x11) != 0) {
                u32 hi = (hw >> 9) & 0x3FF;
                u32 v98 = ((hi | 0x98000000) & ~0x3FC00u) | (((u32)*(u8*)(arg1 + 0x15) << 10) & 0x3FC00);
                GxBpWrite(v98);
                GxBpWrite(0xFEFFFF00);
                GxBpWrite(0x0F000000);
                GxBpWrite(((*(u32*)(arg1 + 0x20) >> 5) & 0xFFFFFF) | 0x64000000);
                GxBpWrite(((hi | 0x65000000) & ~0x1FC00u) |
                          ((((u32)*(u8*)(arg1 + 0x11) >> 1) << 10) & 0x1FC00));
                GxBpWrite(0xFEFFFF00);
                GxBpWrite(0x0F000000);
            }
        }
        GxBpWrite(((u32)*(u16*)(arg1 + 0x8) - 1) | 0x30000000);
        GxBpWrite(((u32)*(u16*)(arg1 + 0xA) - 1) | 0x31000000);
        return;
    }
    if (lbl_805FB50C & 2) {
        u8 fl = lbl_805FB50C;
        lbl_805FB50C = fl & ~2;
        if (fl & 1) {
            GxBpWrite(0xFE00000F);
            GxBpWrite(0);
            GxXfWrite(0x103F, 0);
            GxBpWrite(0xFE003C00);
            GxBpWrite(0);
            GxXfWrite(0x1010, 0x441);
            GxBpWrite(0x28380000);
            GxBpWrite(0xC008F2A4);
            GxBpWrite(0xC108E6F0);
            return;
        }
        GxBpWrite(0xFE00000F);
        GxBpWrite(0);
        GxXfWrite(0x103F, 0);
        GxBpWrite(0xFE003C00);
        GxBpWrite(0);
        GxXfWrite(0x100E, 0x442);
        GxXfWrite(0x1010, 0x442);
        GxBpWrite(0x28380000);
        GxBpWrite(0xC008FFFA);
        GxBpWrite(0xC108FFD0);
    }
}
// 0x80078BB8
// Builds the per-slot table of a 0x258-based record array: for each of arg0[0x8c] requested
// (a, b) pairs, finds b in the owner's list at +0xFD4 (stride 0x50 per a), sets up its record
// through fn_8007C5FC (once per pair), then points arg0[0x90+i] at the record and passes a
// 50.0 / total-count value to fn_8007D2BC.
void fn_80078BB8(u8* arg0) {
    s32 spE8[0x14];   // per-request b index
    s32 sp98[0x14];   // per-request a index
    s32 sp138[0x3C];  // record slot per (a, b), -1 = unset
    s32 sp48[0x14];   // per-record counts
    char sp8[0x40];
    fn_800782AC(arg0, spE8, sp98);
    memset(sp138, 0xFF, 0xF0);
    s32 count = *(s32*)(arg0 + 0x8C);
    *(s32*)(arg0 + 0x254) = 0;
    for (s32 i = 0; i < count; i++) sp48[i] = 0;
    for (s32 i = 0; i < *(s32*)(arg0 + 0x8C); i++) {
        s32 a = sp98[i];
        s32 b = spE8[i];
        if (sp138[a * 0x14 + b] != -1) continue;
        s32 found = -1;
        for (s32 k = 0; k < 20; k++) {
            if (b == *(s32*)(arg0 + a * 0x50 + 0xFD4 + k * 4)) {
                found = k;
                break;
            }
        }
        if (found < 0) SysSprintf(sp8, lbl_8035ABC0, b, a);
        u8* base = arg0 + a * 0x50 + found * 4;
        u8* rec = arg0 + a * 0xF0 + found * 0xC;
        s32 n = fn_8007C5FC(arg0 + (*(s32*)(arg0 + 0x254) << 6) + 0x258,
                            *(s32*)(base + 0x854), *(s32*)(base + 0x764), *(s32*)(base + 0x944),
                            rec + 0xA34, rec + 0xD04);
        s32 slot = *(s32*)(arg0 + 0x254);
        sp138[a * 0x14 + b] = slot;
        sp48[slot] += n;
        *(s32*)(arg0 + 0x254) = *(s32*)(arg0 + 0x254) + 1;
    }
    s32 total = 0;
    for (s32 i = 0; i < *(s32*)(arg0 + 0x8C); i++) {
        s32 slot = sp138[sp98[i] * 0x14 + spE8[i]];
        *(u8**)(arg0 + 0x90 + i * 4) = arg0 + (slot << 6) + 0x258;
        total += sp48[sp138[sp98[i] * 0x14 + spE8[i]]];
    }
    total += fn_8007C5FC(arg0 + 0x1D0, *(s32*)(arg0 + 0x10C8), *(s32*)(arg0 + 0x10C4),
                         *(s32*)(arg0 + 0x10CC), arg0 + 0x10D0, arg0 + 0x10DC);
    total += fn_8007C5FC(arg0 + 0x210, *(s32*)(arg0 + 0x10EC), *(s32*)(arg0 + 0x10E8),
                         *(s32*)(arg0 + 0x10F0), arg0 + 0x10F4, arg0 + 0x1100);
    fn_8007D2BC(*(u8**)(arg0 + 0x760), lbl_805FD6A8 / (f32)total);
}
// 0x80082474
// Per-frame update of a chasing/steering entity: derives the offset to the player, steers the
// orientation, runs the optional pose/target logic, and finally writes the new position.
void fn_80082474(u8* arg0, s32 arg1, f32 farg0) {
    f32 spF8[3];
    Vec3 spEC;
    Vec3 spE0;
    f32 spD0[4];
    Vec3 spC0;
    f32 spB0[4];
    f32 spA0[4];
    Vec3 sp90;
    f32 sp84[3];
    f32 sp78[3];
    Vec3 sp6C;
    Vec3 sp60;
    f32 sp50[4];
    f32 sp44[3];
    f32 sp38[3];
    f32 sp28[4];
    f32 sp18[4];
    u8* sp14;
    Scalar sp10;
    Scalar spC;
    Scalar sp8;
    f32 sp108[12];

    if (*(s32*)(arg0 + 0x264) == 2) {
        fn_80084D3C(spF8, (u8*)(intptr_t)fn_8008294C(*(u8**)(arg0 + 0x2C0)));
        fn_80082120(arg0, spF8);
    }
    f32 t = *(f32*)(arg0 + 0x274);
    f32 lim = *(f32*)(arg0 + 0x27C);
    if (t >= lim) t = lim;
    fn_80082954(&spEC, arg0, t);
    u8* player = fn_80081A34();
    Vec3Sub(&spE0, &spEC, (const Vec3*)(player + 8));
    Vec3SetZ(&spE0, lbl_805FD800);
    f32 dist = fn_8007DD7C(&spE0);
    *(u8*)(arg0 + 0x2BD) = (dist >= fn_80082A04(*(u8**)(arg0 + 0x2C0)));
    if (fn_80082A0C(arg0, 2) == 0) {
        fn_8005263C(spD0);
        ScalarSet(&sp10, farg0);
        Vec3MulScalar(&spC0, (const Vec3*)(arg0 + 0x2A4), &sp10);
        fn_80082A20((u8*)spD0, Vec3GetX(&spC0));
        fn_80082A2C((u8*)spD0, Vec3GetY(&spC0));
        fn_80082A38((u8*)spD0, Vec3GetZ(&spC0));
        fn_80082A44((u8*)spD0, lbl_805FD800);
        fn_80082A50((u8*)spB0, (u8*)(uintptr_t)fn_800533A4(arg0));
        fn_80054530(spA0, spD0, spB0);
        ScalarSet(&spC, lbl_805FD808);
        fn_80082A74(spA0, &spC);
        fn_80082AC4(spB0, spA0);
        fn_8005484C(spB0);
        fn_80082B20(arg0, spB0);
    }
    fn_801B6F34(arg0);
    s32 handled = 0;
    if (*(f32*)(arg0 + 0x274) >= *(f32*)(arg0 + 0x278) && arg1 == 0) {
        handled = fn_80082F9C(arg0, (u8*)(intptr_t)fn_8008294C(*(u8**)(arg0 + 0x2C0)));
        if (handled == 0) {
            handled = fn_80083314(arg0, fn_80082B88(*(u8**)(arg0 + 0x2C0)));
        }
    }
    if ((*(f32*)(arg0 + 0x274) >= *(f32*)(arg0 + 0x27C) || *(u8*)(arg0 + 0x2BD) != 0) && handled == 0) {
        *(s32*)(arg0 + 0x2B8) = 2;
        if (*(u8*)(arg0 + 0x2BD) != 0 || fn_80082A0C(arg0, 1) == 0) {
            Vec3SetZ(&spEC, lbl_805FD82C);
            if (*(u8*)(arg0 + 0x2BD) != 0) {
                ScalarSet(&sp8, lbl_805FD830 * fn_80082A04(*(u8**)(arg0 + 0x2C0)));
                Vec3NormalizeTo(&sp60, &spE0);
                Vec3MulScalar(&sp6C, &sp60, &sp8);
                fn_80054484(&sp90, (const Vec3*)(player + 8), &sp6C);
                Vec3SetX(&spEC, Vec3GetX(&sp90));
                Vec3SetY(&spEC, Vec3GetY(&sp90));
                Vec3SetZ(&spEC, Vec3GetZ((const Vec3*)(intptr_t)fn_800526DC(arg0)));
                if (fn_80082A0C(arg0, 1) != 0) {
                    fn_80082B90(arg0, 0);
                }
            }
            fn_80081E24(arg0, (f32*)&spEC);
            fn_80053208((u8*)sp50);
            fn_80082B20(arg0, sp50);
            EntitySyncTransform(arg0, 0);
            fn_801EC080(arg0 + 0x94, arg0);
            u32 idx = fn_801EC880(arg0 + 0x94, 0x8E78D2A9u);
            sp14 = NULL;
            u32 hash = 0x0B478219u;
            u8* entry = *(u8**)(arg0 + 0x98) + idx * 0xC8;
            if (*(s32*)(arg0 + 0x264) == 3) hash = 0xC467864Eu;
            fn_80064568(lbl_8047C760, hash, (const f32*)fn_800526D4(entry + 0x38), &sp14);
            fn_80054AC0(sp14, (const Vec3*)fn_800526D4(entry + 0x38));
            fn_800359F4((u8*)sp84, lbl_805FD800, lbl_805FD824, lbl_805FD800);
            fn_80082C04(sp14, sp84);
            fn_80082D88(sp14, sp84);
            if (*(u8*)(arg0 + 0x2BD) != 0) {
                fn_800557B4((u8*)sp108);
                fn_80082F0C((u8*)sp44, lbl_805FD824);
                fn_80144624((u8*)sp108, (const Vec3*)sp44, lbl_805FD834);
                for (s32 i = 0; i < *(s32*)(sp14 + 0x10); i++) {
                    u8* e = *(u8**)(sp14 + 0x14 + i * 4);
                    if (e != NULL) {
                        fn_80082F20((u8*)sp38, fn_80082F3C(*(u8**)(e + 0x5C)));
                        fn_80055828(sp78, (u8*)sp108, (const Vec3*)sp38);
                        fn_80082BE0((u8*)sp18, (u8*)sp78);
                        fn_80082F68(sp28, sp18);
                        fn_80082F44(e, (u8*)sp28);
                    }
                }
            }
            fn_80082114(arg0);
            return;
        }
        fn_80081E24(arg0, (f32*)&spEC);
        return;
    }
    fn_80081E24(arg0, (f32*)&spEC);
}
// Record accessor with the original's clamp quirk: an index outside [0, count] yields the
// table itself instead of an entry.
static u8* SpawnRecord(u8* tbl, s32 idx) {
    s32 count = *(s32*)(tbl + 0x320);
    s32 c = idx < 0 ? 0 : (idx > count ? count : idx);
    return idx != c ? tbl : tbl + idx * 0x28;
}

// 0x800D795C
// Spawns the monster described by spawn record arg1 of table arg0 (database, random or plain
// spawn at its node), re-applies the movement AI for certain monster kinds, then restores the
// saved word at +8.
void fn_800D795C(u8* arg0, s32 arg1) {
    *(u8*)(SpawnRecord(arg0, arg1) + 0x20) = 0;
    void* ch = CharacterFind(*(s32*)SpawnRecord(arg0, arg1));
    if (ch == NULL) return;
    s32 saved = *(s32*)(SpawnRecord(arg0, arg1) + 0x8);
    u8* r = SpawnRecord(arg0, arg1);
    if (*(s32*)(r + 0x1C) != -1) {
        SpawnDatabaseMonsterAtNode(ch, *(u32*)(r + 0x1C), *(u32*)(r + 0x4),
                                   (s32)*(f32*)(r + 0xC), (s32)*(f32*)(r + 0x10));
    } else if (*(s32*)(r + 0x18) != -1 && *(s32*)(r + 0x4) != 0) {
        SpawnRandomMonsterAtNode(ch, *(u32*)(r + 0x18), *(u32*)(r + 0x4),
                                 (s32)*(f32*)(r + 0xC), (s32)*(f32*)(r + 0x10));
    } else if (*(s32*)(r + 0x4) != 0) {
        SpawnMonsterAtNode(ch, *(u32*)(r + 0x4), (s32)*(f32*)(r + 0xC), (s32)*(f32*)(r + 0x10));
        if (*(u32*)(*(u8**)((u8*)ch + 0x4) + 0x1C) & 0x10) {
            u32 kind = *(u32*)((u8*)ch + 0x6B4);
            if (kind == 0x4B9FE6EBu || kind == 0x711A757Du) {
                SetEntityMovementAI(ch, 0xE6BE4FBEu, 0);
            }
        }
    }
    if (saved != 0) {
        *(s32*)(SpawnRecord(arg0, arg1) + 0x8) = saved;
    }
}
// 0x800ECCCC
// Launches a projectile helper from self towards target: picks the start position from one of
// the locator records selected by info (or self's own position), jitters the aim direction by a
// random angle when spread is non-zero, rotates it by the helper's pitch and hands it to
// fn_80065200 with its speed scale.
void fn_800ECCCC(u8* arg0, void* owner, u8* target, u8* info, s32 flags1, s32 flags2, f32 spread) {
    typedef s32 (*TargetCheckFn)(void*);
    typedef u8* (*TargetPosFn)(void*, u8*, u8*);
    f32 aim[3];        // sp80
    f32 start[3];      // sp74
    f32 dir[3];        // sp5C
    f32 loc[12];       // sp90: locator record (+0x24..+0x2C = position)
    u8* helper = (u8*)SpawnHelper(owner, target);
    if (helper == NULL) return;
    u16 infoFlags = *(u16*)info;
    f32 pitch = *(f32*)(*(u8**)(helper + 0x228) + 0xD8);
    aim[0] = lbl_805FEB0C;
    aim[1] = lbl_805FEB0C;
    aim[2] = lbl_805FEB0C;
    s32 idx = (infoFlags & 1) ? (s32)*(u8*)(info + 2) : -1;
    s32 haveLoc;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(arg0)) {
            fn_801B55F4(arg0, (u8*)loc, idx);
            haveLoc = 1;
        } else {
            haveLoc = 0;
        }
    } else {
        idx = (infoFlags & 2) ? (s32)*(u8*)(info + 2) : -1;
        if (idx != -1) {
            u8* e = *(u8**)(arg0 + 0x98) + idx * 0xC8;
            haveLoc = 1;
            for (s32 i = 0; i < 12; i++) loc[i] = *(f32*)(e + 0x38 + i * 4);
        } else {
            haveLoc = 0;
        }
    }
    if (haveLoc != 0) {
        start[0] = loc[9];
        start[1] = loc[10];
        start[2] = loc[11];
    } else {
        start[0] = *(f32*)(arg0 + 0x18);
        start[1] = *(f32*)(arg0 + 0x1C);
        start[2] = *(f32*)(arg0 + 0x20);
    }
    if (target == NULL) return;
    if ((*(TargetCheckFn*)(*(u8**)target + 0x4C))(target) != 0 &&
        !(*(u32*)(*(u8**)(target + 0x4) + 0x1C) & 0x20)) {
        return;
    }
    f32* tp = (f32*)(*(TargetPosFn*)(*(u8**)target + 0x44))(target, arg0 + 0x18, arg0);
    aim[0] = tp[0];
    aim[1] = tp[1];
    aim[2] = tp[2];
    s32 noSpread = 0;
    if (spread > lbl_805FEB54 && spread < lbl_805FEB58) noSpread = 1;
    if (noSpread == 0) {
        // random direction in the XY plane, scaled by spread
        f32 ang = lbl_805FEB0C + lbl_805FEB5C * (lbl_805FEB1C * (f32)RandomU32());
        f32 sn = sinf(lbl_805FEB50 * ang);
        f32 cs = cosf(lbl_805FEB50 * ang);
        aim[0] += cs * spread;
        aim[1] += sn * spread;
        aim[2] += lbl_805FEB0C * spread;
    }
    f32 dx = aim[0] - start[0];
    f32 dy = aim[1] - start[1];
    f32 dz = aim[2] - start[2];
    aim[0] = dx;
    aim[1] = dy;
    aim[2] = dz;
    Vec3 n;
    if (dz * dz + (dx * dx + dy * dy) == 0.0f) {
        n.x = 0.0f;
        n.y = 0.0f;
        n.z = 0.0f;
    } else {
        n = Vec3Normalized(Vec3{ dx, dy, dz });
    }
    dir[0] = n.x;
    dir[1] = n.y;
    dir[2] = n.z;
    if (pitch != lbl_805FEB0C) {
        f32 ang = lbl_805FEB50 * pitch;
        f32 cs = cosf(ang);
        f32 sn = sinf(ang);
        f32 x = dir[0];
        f32 y = dir[1];
        f32 z = dir[2];
        dir[0] = x * cs + lbl_805FEB0C * sn;
        dir[1] = y * cs + lbl_805FEB0C * sn;
        dir[2] = z * cs + lbl_805FEB04 * sn;
    }
    f32 speed = *(f32*)(*(u8**)(helper + 0x228) + 0xB8);
    dir[0] *= speed;
    dir[1] *= speed;
    dir[2] *= speed;
    fn_80065200(helper, (u8*)start, dir, arg0, 0, flags1 | 0x4000, flags2 | 0x4000);
}
// Looks up the display string for a text id (id -1 = "not found"), using the high-bit table
// gStringTable2 for ids with bit 31 set and gStringTable otherwise.
static const char* IngredientText(s32 id) {
    if (id == -1) return lbl_80354800;
    if ((id & 0x80000000) && gStringTable2 != NULL) {
        u8* t2 = (u8*)gStringTable2;
        const char** arr = *(const char***)t2;
        u32 i = (u32)id & 0x7FFFFFFFu;
        if (arr != NULL && i < *(u32*)(t2 + 0xC)) return arr[i];
        return lbl_805F8200;
    }
    u8* t = (u8*)gStringTable;
    const char** arr = *(const char***)t;
    if (arr != NULL && (u32)id < *(u32*)(t + 0xC)) return arr[id];
    if (id == -1) return lbl_8035480C;
    return lbl_805F8200;
}

// Finds the icon for ingredient/database entry idx of kind type (0: table 0x4F00, 1: table
// 0x5328): formats its path, retrying with the fallback format if it is not found.
static u8* IngredientIcon(s32 type, s32 idx, char* buf) {
    u8* data = gGameDb->data;
    if (type == 0) {
        u8* r = *(u8**)(data + 0x4F00) + idx * 0x10;
        SysSprintf(buf, lbl_8036D060 + 0x14, lbl_805F8F10, (s32)*(s8*)(r + 0xA), *(s32*)(r + 4),
                   *(s32*)(r + 0));
        u8* icon = fn_801094F4(buf, 0);
        if (icon == NULL) {
            SysSprintf(buf, lbl_8036D060 + 0x2C, lbl_805F8F10);
            icon = fn_801094F4(buf, 0);
        }
        return icon;
    }
    if (type == 1) {
        u8* r = *(u8**)(data + 0x5328) + idx * 0x18;
        SysSprintf(buf, lbl_8036D060 + 0x4C, lbl_805F8F10, (s32)*(s8*)(r + 0x12), *(s32*)(r + 0xC),
                   *(s32*)(r + 8), *(s32*)(r + 4), *(s32*)(r + 0));
        u8* icon = fn_801094F4(buf, 0);
        if (icon == NULL) {
            SysSprintf(buf, lbl_8036D060 + 0x2C, lbl_805F8F10);
            icon = fn_801094F4(buf, 0);
        }
        return icon;
    }
    return NULL;
}

// 0x8011813C
// Builds the ingredient list entries (0x20 bytes each from arg0, count at +0x34) from the 0x69
// save records that match the list kind arg0[0x28] (and arg0[0x20] where it applies).
void fn_8011813C(u8* arg0) {
    char buf[0x100];
    *(s32*)(arg0 + 0x34) = 0;
    for (s32 i = 0; i < 0x69; i++) {
        u8* rec = lbl_805FBBE0 + i * 0xC;
        s32 type = *(s32*)(rec + 0xBE8);
        if (type != *(s32*)(arg0 + 0x28)) continue;
        if (type == 1) {
            u8* d = *(u8**)(gGameDb->data + 0x5328) + *(s32*)(rec + 0xBEC) * 0x18;
            s32 want = *(s32*)(arg0 + 0x20);
            if (*(s32*)(d + 0xC) != want && want != 3) continue;
        }
        if (*(u8*)(rec + 0xBF0) == 0) continue;
        if (type == 0) {
            u8* e = arg0 + (*(s32*)(arg0 + 0x34) << 5);
            u8* d = *(u8**)(gGameDb->data + 0x4F00) + *(s32*)(rec + 0xBEC) * 0x10;
            *(s32*)(e + 0x38) = -1;
            *(s32*)(e + 0x3C) = *(s32*)(rec + 0xBE8);
            *(u32*)(e + 0x4C) = *(u32*)(d + 4);
            *(const char**)(e + 0x50) = IngredientText(TextIdFromHash(*(u32*)(d + 4)));
            *(u8**)(e + 0x48) = IngredientIcon(*(s32*)(rec + 0xBE8), *(s32*)(rec + 0xBEC), buf);
        } else if (type == 1) {
            u8* d = *(u8**)(gGameDb->data + 0x5328) + *(s32*)(rec + 0xBEC) * 0x18;
            u8* e = arg0 + (*(s32*)(arg0 + 0x34) << 5);
            *(s32*)(e + 0x38) = *(s32*)(d + 0xC);
            *(s32*)(e + 0x3C) = *(s32*)(rec + 0xBE8);
            *(u32*)(e + 0x4C) = *(u32*)(d + 4);
            *(const char**)(e + 0x50) = IngredientText(TextIdFromHash(*(u32*)(d + 4)));
            *(u8**)(e + 0x48) = IngredientIcon(*(s32*)(rec + 0xBE8), *(s32*)(rec + 0xBEC), buf);
        }
        u8* e = arg0 + (*(s32*)(arg0 + 0x34) << 5);
        *(s32*)(e + 0x40) = *(s32*)(rec + 0xBEC);
        SysSprintf((char*)(e + 0x54), lbl_805F8F14, (s32)*(u8*)(rec + 0xBF0));
        *(s32*)(arg0 + 0x34) = *(s32*)(arg0 + 0x34) + 1;
    }
}
// Display list i of the ring (0x94 bytes each) and its submission record (0x14 bytes each).
static RendDL* DlAt(s32 i) { return (RendDL*)(lbl_80401108 + i * 0x94); }
static RendDLRecord* DlRecAt(s32 i) { return (RendDLRecord*)(lbl_80400FF0 + 0xAEC + i * 0x14); }

// 0x80154974
// Resets the GX matrix scratch area (identity at +0xE8, +0xB8; copy +0x58 -> +0x88), makes
// sure the current display list has room (closing it, submitting it and opening the next one
// when it does not), uploads the transposed +0x88 matrix as an XF matrix and re-identities +0xB8.
void fn_80154974(void) {
    static const s32 kTranspose[12] = { 0, 3, 6, 9, 1, 4, 7, 10, 2, 5, 8, 11 };
    f32* m58 = (f32*)(lbl_80400FF0 + 0x58);
    f32* m88 = (f32*)(lbl_80400FF0 + 0x88);
    f32* mE8 = (f32*)(lbl_80400FF0 + 0xE8);
    f32* m10 = (f32*)(lbl_80400FF0 + 0x10);
    f32* mB8 = (f32*)(lbl_80400FF0 + 0xB8);
    for (s32 i = 0; i < 12; i++) mE8[i] = (i % 4 == 0) ? 1.0f : 0.0f;  // 3x4 identity
    for (s32 i = 0; i < 12; i++) m88[i] = m58[i];
    if (fn_8015BA30(DlAt(lbl_805FB4BC)) + 0x10FC > lbl_805FB4C8) {
        if (lbl_805FB4C0 != 0) {
            RendDLEnd(DlAt(lbl_805FB4BC));
            s32 rest = 0x24000 - lbl_805FB4C8;
            if (rest != 0) {
                fn_80142ED0(RendDLGetBase(DlAt(lbl_805FB4BC)) + lbl_805FB4C8, rest);
            }
            if (lbl_805FB4D4 != 0) {
                lbl_805FB4DC += 1;
            } else {
                RendThreadSubmitDL(DlRecAt(lbl_805FB4BC), lbl_805FB4E0 == 0);
            }
            lbl_805FB4C0 = 0;
            lbl_805FB4D0 = NULL;
            lbl_805FB4C4 = 0;
            lbl_805FB4C8 = 0;
            lbl_805FB4CC = 0;
            if (lbl_805FB4E0 != 0) lbl_805FB4E8 += 1;
            s32 next = lbl_805FB4BC + 1;
            lbl_805FB4BC = next;
            if (next >= 0x11) lbl_805FB4BC = 0;
            if (lbl_805FB4D4 != 0 && RendHasFreeDL() == 0 && lbl_805FB4D4 != 0) {
                RendThreadSync();
                s32 head = lbl_805FB4D8;
                s32 count = lbl_805FB4DC;
                while (count > 0) {
                    RendThreadSubmitDL(DlRecAt(head), lbl_805FB4E0 == 0);
                    head = lbl_805FB4D8 + 1;
                    lbl_805FB4D8 = head;
                    if (head >= 0x11) {
                        lbl_805FB4D8 = 0;
                        head = 0;
                    }
                    count = lbl_805FB4DC - 1;
                    lbl_805FB4DC = count;
                }
                lbl_805FB4D4 = 0;
                lbl_805FB4D8 = 0;
                lbl_805FB4DC = 0;
            }
            RendWaitFreeDL();
            RendDLBegin(DlAt(lbl_805FB4BC), DlRecAt(lbl_805FB4BC));
            lbl_805FB4C0 = 1;
            lbl_805FB4C4 = 0x24000;
            lbl_805FB4C8 = 0x24000;
            lbl_805FB4CC = 0;
            lbl_805FB4D0 = RendDLGetBase(DlAt(lbl_805FB4BC));
            lbl_805FB4EC -= 1;
        }
        fn_8015BA30(DlAt(lbl_805FB4BC));
    }
    // transpose the 3x4 matrix at +0x88 into +0x10 and upload it as XF matrix 0 (12 words)
    f32 t[12];
    for (s32 i = 0; i < 12; i++) t[i] = m88[kTranspose[i]];
    for (s32 i = 0; i < 12; i++) m10[i] = t[i];
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0xB;
    GXWGFifo._u16 = 0;
    for (s32 i = 0; i < 12; i++) GXWGFifo._f32 = m10[i];
    for (s32 i = 0; i < 12; i++) mB8[i] = (i % 4 == 0) ? 1.0f : 0.0f;
    fn_80154398();
}
// 0x8012F8B8
// Initialises an entity's type-specific parts: runs the constructor of the entity type (arg2,
// or the type byte of its child when arg1 is set), re-runs the attached helper objects (+0x90,
// +0xA4), propagates to the sub-objects, marks the child, and recurses over the child list
// at +0xB0.
void fn_8012F8B8(u8* obj, s32 arg1, s32 arg2) {
    if (obj == NULL) return;
    if (arg1 != 0 && *(void**)(obj + 0x4) == NULL) return;
    u32 type;
    if (arg1 != 0) {
        type = *(u8*)(*(u8**)(obj + 0x4) + 0x34);
    } else {
        type = (u32)arg2;
    }
    switch (type) {
    case 2:
        fn_8006F61C(obj);
        break;
    case 3:
        fn_8005CA88(obj);
        break;
    case 4:
        fn_80052530(obj, (s32)type, NULL, 0);
        break;
    case 5:
        fn_8005CC30(obj, (s32)type, NULL, 0);
        break;
    case 15:
        fn_8006CCC4(obj, (s32)type, NULL, 0);
        break;
    case 16:
        fn_8006CFA4(obj, (s32)type, NULL, 0);
        break;
    case 6: {
        u8* sub = *(u8**)(obj + 0x9C);
        u32 id = sub != NULL ? *(u32*)(sub + 0x8) : 0;
        if (fn_801088C0(obj, id, NULL) == NULL) {
            fn_800690D4(obj, (s32)type, NULL, 0);
        }
        break;
    }
    case 13:
        fn_8004B604(obj);
        break;
    case 14:
        fn_8004B498(obj);
        break;
    case 12:
        fn_8004E684(obj, (s32)type, NULL, 0);
        break;
    case 7:
        fn_800636BC(obj);
        break;
    case 8:
        fn_8006D2D0(obj);
        break;
    case 9:
        fn_8006F6BC(obj);
        break;
    case 10:
        fn_8006307C(obj);
        break;
    default:
        fn_801B4334(obj, 0, NULL, 0);  // r4-r6 forwarded from caller in asm
        break;
    }

    if (arg1 != 0) {
        u8* sub = *(u8**)(obj + 0x90);
        if (sub != NULL) {
            switch (*(s32*)(sub + 0x4)) {
            case 1:
                fn_801D94CC(sub);
                break;
            case 2:
                fn_801DA81C(sub);
                fn_801DB160(sub);
                break;
            case 3:
                fn_801DA1CC(sub);
                fn_801DA7C8(sub);
                break;
            case 4:
                fn_801D9A24(sub);
                break;
            default:
                break;
            }
        }
    }

    u8* helper = *(u8**)(obj + 0xA4);
    if (helper != NULL) {
        u8* hc = *(u8**)(helper + 0x4);
        if (hc != NULL) *(u32*)(hc + 0x18) = *(u32*)(lbl_803C4298 + 0x38);
        u8** h0 = *(u8***)helper;
        if (h0 != NULL && *h0 != NULL) fn_801BC574(*h0);
    }
    fn_8012F6B0(*(u8**)(obj + 0x8C));
    if (arg1 != 0) {
        u8* child = *(u8**)(obj + 0x4);
        if ((*(u32*)(child + 0x1C) & 0x40) && *(void**)(obj + 0x194) != NULL) {
            for (s32 i = 0; i < *(s32*)(obj + 0x1A4); i++) {
                fn_8012F6B0(*(u8**)(*(u8**)(obj + 0x194) + i * 4));
            }
        }
        child = *(u8**)(obj + 0x4);
        if (*(u32*)(child + 0x1C) & 0x100) {
            if (gFlag805FB948 != 0) {
                if (!(*(u32*)(obj + 0x1B0) & 0x80)) {
                    u8* c = *(u8**)(obj + 0x4);
                    *(u32*)(c + 0x20) |= 0x80;
                }
            } else {
                if (!(*(u32*)(obj + 0x1B0) & 0x40)) {
                    u8* c = *(u8**)(obj + 0x4);
                    *(u32*)(c + 0x20) |= 0x80;
                }
            }
        }
    }
    for (u8* n = *(u8**)(obj + 0xB0); n != NULL; n = *(u8**)(n + 0x8)) {
        if (*(u8**)(n + 0x4) != NULL) fn_8012F8B8(*(u8**)(n + 0x4), 1, 0);
    }
}

// Bits per pixel of a GX texture format (0 for formats without a plain size).
static s32 TexFmtBits(u32 fmt) {
    static const u8 kBits[25] = { 16, 16, 16, 32, 4, 8, 0, 0, 4, 8, 8, 4, 8,
                                  8, 4, 4, 8, 8, 16, 16, 16, 16, 32, 16, 32 };
    return fmt > 0x18 ? 0 : kBits[fmt];
}

// 0x8014ACCC
// Computes the size of a texture image of format fmt (0..0x18) and size w x h: rounds w/h up to
// the format's block size, then returns the row stride (outStride), total byte size (outSize)
// and the rounded dimensions. mode selects the layout variant (0..6; others use the default).
// A non-positive w or h zeroes all outputs.
void fn_8014ACCC(u32 mode, s32 fmt, s32 w, s32 h, s32* outSize, s32* outStride, s32* outW, s32* outH) {
    s32 stride;
    s32 size;
    if (w < 1 || h < 1) {
        if (outStride) *outStride = 0;
        if (outSize) *outSize = 0;
        if (outW) *outW = 0;
        if (outH) *outH = 0;
        return;
    }
    switch (mode > 6 ? 0 : mode) {
    case 1:
        if ((u32)fmt <= 10) {
            switch (fmt) {
            case 0: case 1: case 2: w = (w + 7) & ~7; break;
            case 3: w = (w + 3) & ~3; break;
            case 4: w = (w + 0x1F) & ~0x1F; break;
            case 5: w = (w + 0xF) & ~0xF; break;
            case 8: w = (w + 7) & ~7; h = (h + 3) & ~3; break;
            case 9: case 10: w = (w + 3) & ~3; h = (h + 3) & ~3; break;
            default: break;
            }
        }
        stride = (w * TexFmtBits((u32)fmt) + 4) / 8;
        size = stride * h;
        break;
    case 4:
        if ((u32)(fmt - 5) <= 0x12) {
            switch (fmt - 5) {
            case 0: case 11: case 12:
                w = (w + 7) & ~7;
                h = (h + 3) & ~3;
                break;
            case 3: case 6: case 10:
                w = (w + 7) & ~7;
                h = (h + 7) & ~7;
                break;
            case 13: case 14: case 17: case 18:
                w = (w + 3) & ~3;
                h = (h + 3) & ~3;
                break;
            default:
                break;
            }
        }
        stride = (w * TexFmtBits((u32)fmt) + 4) / 8;
        size = (stride * h + 0x1F) & ~0x1F;
        break;
    case 5:
        if (fmt >= 8 && fmt < 0xB) {
            w = (w + 0x7F) & ~0x7F;
            h = (h + 0x7F) & ~0x7F;
        } else {
            w = (w + 0x1F) & ~0x1F;
            h = (h + 0x1F) & ~0x1F;
        }
        if ((u32)(fmt - 8) <= 2) {
            stride = ((w / 4) * (fmt == 8 ? 8 : 0x10) + 0xFF) & ~0xFF;
            size = (h / 4) * stride;
        } else {
            s32 bits = TexFmtBits((u32)fmt);
            stride = (((w * bits + 4) / 8) + 0xFF) & ~0xFF;
            w = bits != 0 ? (stride * 8) / bits : 0;
            size = stride * h;
        }
        break;
    default:
        // modes 0, 2, 3, 6 and everything above 6 share the plain layout
        if (fmt >= 8 && fmt < 0xB) {
            w = (w + 3) & ~3;
            h = (h + 3) & ~3;
        }
        stride = (w * TexFmtBits((u32)fmt) + 4) / 8;
        size = stride * h;
        break;
    }
    if (outStride) *outStride = stride;
    if (outSize) *outSize = size;
    if (outW) *outW = w;
    if (outH) *outH = h;
}

// 0x80169C6C
// Debug dump of the lists of a resource manager: for each entry of its four lists it runs the
// per-entry pass (fn_8018409C / fn_8016A324 / fn_80175088 / fn_80173568) and, when arg1 is 0,
// formats a description line into a local buffer.
void fn_80169C6C(u8* arg0, s32 arg1) {
    char buf208[0x100];
    char buf108[0x100];
    char buf8[0x100];
    for (u8* n = *(u8**)(arg0 + 0x4); n != NULL; n = *(u8**)n) {
        u8* item = n - 0x28;
        const char* s1 = lbl_805F9260;
        switch (*(u8*)(n - 0x1A)) {
        case 0x10: s1 = lbl_805F9264; break;
        case 0x20: s1 = lbl_805F926C; break;
        case 0x08: s1 = lbl_805F9274; break;
        case 0x04: s1 = lbl_805F927C; break;
        default: break;
        }
        const char* s2 = lbl_805F9260;
        switch (*(u8*)(item + 0xD)) {
        case 0: case 19: s2 = lbl_805F9284; break;
        case 1: case 20: s2 = lbl_805F9290; break;
        case 2: case 21: s2 = lbl_805F9298; break;
        case 4: case 11: s2 = lbl_80376850 + 0x0; break;
        case 5: s2 = lbl_80376850 + 0xC; break;
        case 8: s2 = lbl_805F92A0; break;
        case 9: s2 = lbl_805F92A8; break;
        case 10: s2 = lbl_805F92B0; break;
        case 12: s2 = lbl_80376850 + 0x18; break;
        case 13: s2 = lbl_80376850 + 0x28; break;
        case 14: case 15: case 17: s2 = lbl_80376850 + 0x38; break;
        case 16: case 18: s2 = lbl_80376850 + 0x48; break;
        case 23: s2 = lbl_805F92B8; break;
        default: break;
        }
        const char* s3 = lbl_805F9260;
        if ((s32)*(u8*)(item + 0x11) * 8 > 0) {
            switch (*(u8*)(item + 0x10)) {
            case 0: case 5: s3 = lbl_805F92C0; break;
            case 1: s3 = lbl_805F92C8; break;
            case 2: s3 = lbl_805F92D0; break;
            case 3: s3 = lbl_805F92D8; break;
            case 4: s3 = lbl_80376850 + 0x58; break;
            case 6: s3 = lbl_805F92E0; break;
            case 7: case 8: s3 = lbl_805F92E8; break;
            default: break;
            }
        }
        fn_8018409C(item);
        if (arg1 == 0) {
            SysSprintf(buf208, lbl_80376850 + 0x68, (s32)*(u16*)(item + 0x4), (s32)*(u16*)(item + 0x6),
                       (s32)*(u8*)(item + 0x13), s1, s2, s3);
        }
    }
    for (u8* n = *(u8**)(arg0 + 0x1C); n != NULL; n = *(u8**)n) {
        u8* item = n - 0x4;
        const char* s = lbl_805F9260;
        s32 kind = *(s32*)(n + 0x10);
        if (kind == 1) {
            s = lbl_805F92F4;
        } else if (kind >= 0) {
            s = lbl_805F92F0;
        }
        s32 count = *(s32*)(item + 0x18);
        s32 total = 0;
        for (s32 i = 0; i < count; i++) {
            total += *(s32*)(item + *(s32*)(item + 0x24) + i * 8 + 4);
        }
        fn_8016A324(item);
        if (arg1 == 0) {
            SysSprintf(buf108, lbl_80376850 + 0x78, s, count, total);
        }
    }
    for (u8* n = *(u8**)(arg0 + 0x10); n != NULL; n = *(u8**)n) {
        fn_80175088(n - 0x10);
    }
    for (u8* n = *(u8**)(arg0 + 0x28); n != NULL; n = *(u8**)n) {
        u8* item = n - 0x4;
        fn_80173568(item);
        if (arg1 == 0) {
            const char* s = lbl_805F9300;
            switch (*(s32*)(item + 0x60)) {
            case 1: s = lbl_805F9308; break;
            case 2: s = lbl_805F9310; break;
            case 3: s = lbl_805F9318; break;
            default: break;
            }
            SysSprintf(buf8, lbl_80376850 + 0x90, s, *(s32*)(item + 0x1C), *(s32*)(item + 0x18));
        }
    }
}
}
