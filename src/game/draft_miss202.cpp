// Object constructors in 0x8002xxxx..0x8003xxxx (UI/minigame controller objects) and fn_801CA30C.
#include <math.h>

#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void* MemAllocSmall(u32 size);                                    // 0x801D41CC
u32 RandomU32(void);                                              // 0x80143460
void ZoneLog(void* loader, const char* fmt, ...);
void* memset(void* p, int c, unsigned long n);
s32 rand(void);
u8** FighterGetSlot(u8* f, s32 i);                                // 0x8020E794
void BattleSetFighter(u8* b, s32 side, u8* f, u32 owner);         // 0x80212E84
void FighterStatsSetOwner(u8* stats, void* owner);                // 0x80213F84
u8* fn_80021594(u8* p, s32 type, s32 hash);                       // 0x80021594 (base constructor)
void fn_80030228(u8* p);                                          // 0x80030228
void fn_8003107C(u8* p);                                          // 0x8003107C
void fn_80031090(u8* p, s32 del);                                 // 0x80031090
void fn_80037FB4(u8* p);                       // 0x80037FB4
u8* fn_800496DC(u8* p, s32 a2);                                // 0x800496DC (constructor, returns p)
void fn_8004DD30(u8* p);                                          // 0x8004DD30
void fn_80075264(u8* p, s32 id);                                  // 0x80075264
u8* fn_801B3308(void);                                            // 0x801B3308
void fn_801B3310(u8* p, u8* q);                                   // 0x801B3310
void* fn_801D40E0(s32 n);                                         // 0x801D40E0
const char* fn_801EB708(u32 hash);                                // 0x801EB708
u8* fn_801F61AC(u8* p, s32 a);                                    // 0x801F61AC
u8* fn_801F8110(u8* p, s32 a4);                                   // 0x801F8110
void fn_801F85E4(u8* p, const u8* src, s32 count);                // 0x801F85E4
void fn_801F8F34(u8* p, f32 v);                                   // 0x801F8F34
void fn_801F8FA0(u8* p, f32 v);                                   // 0x801F8FA0
void fn_801F94B0(u8* p, s32 v);                                   // 0x801F94B0
u8* fn_801F9544(u8* p, s32 a4);                                   // 0x801F9544
u8* fn_801F9C2C(u8* p, s32 a4);                                   // 0x801F9C2C
u8* fn_801F9D3C(u8* p, s32 a);                                    // 0x801F9D3C
void* fn_8032F028(void* mem, void* ctor, void* dtor, s32 size, s32 count);  // 0x8032F028
void fn_8032F1E8(u8* arr, void* ctor, void* dtor, s32 size, s32 count);     // 0x8032F1E8
// element constructors / destructors passed to the array helpers
void fn_800259D8(u8* p);
u8* fn_8002AB00(u8* p);
u8* fn_8002AB4C(u8* p, s32 del);
u8* fn_8002B4D0(u8* p);
u8* fn_8002B514(u8* p, s32 del);
u8* fn_80030B6C(u8* p);
u8* fn_80030BC8(u8* p, s32 del);
u8* fn_80030604(u8* p);
u8* fn_80030648(u8* p, s32 del);
u8* fn_800308A4(u8* p);
u8* fn_800308E8(u8* p, s32 del);
u8* fn_80030754(u8* p);
u8* fn_80030798(u8* p, s32 del);
u8* fn_800309F4(u8* p);
u8* fn_80030A40(u8* p, s32 del);
u8* fn_80032EDC(u8* p);
u8* fn_80032F7C(u8* p, s32 del);
}

// defined in small_8003c.cpp (C++ linkage)
extern "C++" {
u8* fn_800389A4(u8* p);
void* fn_800385A8(u8* p, s32 del);
void fn_80038578(u8* p);
void fn_8003CD7C(u8* p);
void* fn_8003CDE0(void* p, s32 del);
}

extern "C++" s32 gZoneResourceSize;  // 0x805FB94C (defined in world_alloc.cpp)

extern "C" {
extern u8* gBattle;                  // 0x805FBBB8
extern u8 gIdPool[];                 // 0x8048A8F0
extern void* gLab;                   // 0x805FBBC8
extern u8 gWorldState[];             // 0x803E6D00
extern u8 lbl_803C4298[];            // level info block
extern u8 lbl_80355C80[];            // vtables
extern u8 lbl_80357408[];
extern u8 lbl_80357528[];
extern u8 lbl_80357880[];
extern u8 lbl_803579D8[];
extern u8 lbl_80357C60[];
extern u8 lbl_80357E80[];
extern u8 lbl_80357F58[];
extern u8 lbl_80358050[];
extern u8 lbl_80358218[];
extern f32 lbl_80347528[];           // float tables
extern f32 lbl_80347538[];
extern f32 lbl_80357748[];
extern s32 lbl_80347620[];
extern f32 lbl_805FC5E8[];
extern f32 lbl_805FC5F0[];
extern s32 lbl_803C4E28[];
extern s32 lbl_805FAF9C;
extern s32 lbl_805FAFA0;
extern s32 lbl_805FAFA4;
extern void* lbl_805FAFB0;
extern void* lbl_805FAFB8;
extern void* lbl_805FAFC0;
extern u8* lbl_805FAFD8;
extern void* lbl_805FAFE0;
extern u8* lbl_805FAFE8;
extern u8* lbl_805FB008;
extern u8* lbl_805FB01C;
extern u8* lbl_805FB020;
extern u8 lbl_805FB024;
extern s32 lbl_805FB034;
extern s32 lbl_805FB038;
extern u8* lbl_805FBBE0;
}

// Finds the zone object whose name hash (+0x9C -> +8) equals `hash`; stores the last match in *out.
static void FindZoneObj(u8** out, u32 hash) {
    u8* rec = *(u8**)(gWorldState + 0x1A4) + (gZoneResourceSize * 2 - gZoneResourceSize);
    for (s32 i = 0; i < *(s32*)(gWorldState + 4); i++) {
        for (s32 j = 0; j < *(s32*)(rec + 0x98); j++) {
            u8* obj = (*(u8***)(rec + 0x12C))[j];
            u8* nm = *(u8**)(obj + 0x9C);
            u32 h = nm != NULL ? *(u32*)(nm + 8) : 0;
            if (h == hash) *out = obj;
        }
    }
}

extern "C" {

// 0x80023884
// Constructor of the digging-minigame controller (UI_MSA_NewDigging): clears state, sets up
// three sub-objects, picks a random target slot, finds the "diggingA" zone object.
u8* fn_80023884(u8* p) {
    fn_80021594(p, 0, 0x9A5E553B);
    *(s32*)(p + 0x90) = 0;
    *(void**)(p + 4) = lbl_80355C80;
    *(s32*)(p + 0x94) = 0;
    *(s32*)(p + 0x98) = 0;
    *(s32*)(p + 0x9C) = 0;
    *(s32*)(p + 0xA0) = 0;
    *(s32*)(p + 0xA4) = 0;
    *(s32*)(p + 0xA8) = 0;
    *(s32*)(p + 0xB8) = 0;
    *(u8*)(p + 0xBC) = 0;
    *(s32*)(p + 0xC0) = 0;
    *(s32*)(p + 0xC4) = 0;
    *(f32*)(p + 0xC8) = 0.0f;
    *(f32*)(p + 0xCC) = 0.0f;
    *(f32*)(p + 0xD0) = 0.0f;
    *(f32*)(p + 0xD4) = 0.0f;
    *(f32*)(p + 0xD8) = 0.0f;
    *(f32*)(p + 0xDC) = 0.0f;
    *(f32*)(p + 0xE0) = 0.0f;
    *(u8*)(p + 0xE4) = 0;
    *(s32*)(p + 0xE8) = 0;
    *(s32*)(p + 0xEC) = 0;
    *(s32*)(p + 0xF4) = 0;
    *(s32*)(p + 0xF8) = 0;
    *(u8*)(p + 0xFC) = 0;
    *(u8*)(p + 0xFD) = 0;
    *(s32*)(p + 0x100) = 0;
    *(u8*)(p + 0x110) = 0;
    fn_801F8110(p + 0x114, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F8110(p + 0x598, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F61AC(p + 0xA1C, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(p + 0xAD8) = 0;
    fn_8003107C(p + 0xAE0);
    *(u8*)(p + 0xAEC) = 0;
    *(u8*)(p + 0xAED) = 0;
    *(s32*)(p + 0xAF0) = 0;
    *(s32*)(p + 0xAF4) = 0;
    *(s32*)(p + 0xAF8) = 0;
    *(s32*)(p + 0xAFC) = 0;
    *(u8*)(p + 0xB10) = 0;
    *(u8*)(p + 0xB11) = 0;
    lbl_805FAFB0 = p;
    lbl_805FAFA0 = 0x6DBCBC22;
    *(void**)(p + 0xADC) = fn_8032F028(MemAllocSmall(4 * 0xC + 0x10), (void*)fn_8003107C, (void*)fn_80031090, 0xC, 4);
    *(s32*)(p + 0xE8) = 4;
    *(s32*)(p + 0xF8) = 0x17 / 4 + 1;
    fn_801B3310(fn_801B3308(), p + 0xA1C);
    *(s32*)(p + 0x1AC) = 1;
    *(f32*)(p + 0x1B0) = 0.0f;
    *(f32*)(p + 0x1B4) = -1.0f;
    *(f32*)(p + 0x1B8) = 0.0f;
    fn_801B3310(fn_801B3308(), p + 0x114);
    *(s32*)(p + 0x630) = 0;
    *(f32*)(p + 0x634) = 0.0f;
    *(f32*)(p + 0x638) = -1.0f;
    *(f32*)(p + 0x63C) = 0.0f;
    *(f32*)(p + 0x62C) = 0.00001f;
    fn_801F8FA0(p + 0x598, 1e-7f);
    fn_801F8F34(p + 0x598, 0.000001f);
    fn_801B3310(fn_801B3308(), p + 0x598);
    u8* g = (u8*)lbl_805FAFB0;
    *(s32*)(g + 0xEC) = *(s32*)(g + 0xF8);
    *(s32*)(g + 0xF0) = (*(s32*)(g + 0xEC) * *(s32*)(g + 0xE8)) / 2;
    g = (u8*)lbl_805FAFB0;
    *(u8*)(g + 0xBC) = 0;
    *(u8*)(g + 0xBD) = 1;
    s32 r = rand();
    *(s32*)(g + 0xF4) = *(s32*)(g + 0xF0) - r % 8;
    *(s32*)(g + 0xC0) = 6;
    *(u8*)((u8*)lbl_805FAFB0 + 0xBC) = 0;
    g = (u8*)lbl_805FAFB0;
    *(s32*)(g + 0xEC) = *(s32*)(g + 0xF8);
    if (*(s32*)(g + 0xF4) == *(s32*)(g + 0xEC)) {
        *(s32*)(g + 0xEC) += 1;
    }
    FindZoneObj((u8**)(p + 0xB8), 0xC31382E0u);  // "diggingA"
    *(f32*)(p + 0x104) = 1.0f;
    *(f32*)(p + 0x108) = 0.0f;
    *(f32*)(p + 0x10C) = 0.0f;
    fn_80075264(p, 0x3CB);
    *(s32*)(p + 8) = 0x32F;
    *(s32*)(p + 0xC) = 0x330;
    *(u32*)(p + 0x30) = 0xB12612F7u;
    *(u32*)(p + 0x34) = 0x282F434Du;
    return p;
}

// 0x80025A28
// Constructor of the skull-puzzle controller (UI_MSA_SkullPuzzle): clears state, replaces the
// global instance (deleting the old one), finds the "ancient_puzzleA" zone object.
u8* fn_80025A28(u8* p) {
    fn_80021594(p, 2, 0x0F2194DF);
    *(void**)(p + 4) = lbl_80357408;
    *(u8*)(p + 0x90) = 0;
    *(s32*)(p + 0x94) = 0;
    *(s32*)(p + 0x98) = -1;
    *(s32*)(p + 0xB0) = 0;
    *(s32*)(p + 0xB4) = 0;
    *(u8*)(p + 0xB8) = 0;
    *(u8*)(p + 0xB9) = 0;
    *(s32*)(p + 0xBC) = 1;
    *(s32*)(p + 0xC0) = 0;
    *(s32*)(p + 0xC4) = 0;
    *(s32*)(p + 0xC8) = 0;
    fn_801F61AC(p + 0xCC, *(s32*)(lbl_803C4298 + 0x38));
    if (lbl_805FAFB8 != NULL) {
        void* o = lbl_805FAFB8;
        (*(void (**)(void*, s32))(*(u8**)((u8*)o + 4) + 8))(o, 1);
    }
    lbl_805FAFB8 = p;
    lbl_805FAFA0 = 0x7B794FF5;
    FindZoneObj((u8**)(p + 0x94), 0x5B78F414u);  // "ancient_puzzleA"
    *(void**)(p + 0xB0) = fn_8032F028(MemAllocSmall(0x250), (void*)fn_800259D8, NULL, 0x48, 8);
    memset(p + 0x18C, 0, 0x50);
    fn_80075264(p, 0x3C1);
    *(s32*)(p + 8) = 0x14F;
    *(s32*)(p + 0xC) = 0x150;
    *(u32*)(p + 0x30) = 0x9DCDDE25u;
    *(u32*)(p + 0x34) = 0x04C48F9Fu;
    memset(p + 0x1DC, 0, 0x18);
    memset(p + 0x1F4, 0, 0x3C);
    return p;
}

// 0x80028038
// Constructor of a controller object (hash 0x5D705BF5): clears state, finds the zone object with
// name hash 0xD716DA31, allocates two object arrays (0x128 / 0x12C byte elements) and a 0xC-byte helper.
u8* fn_80028038(u8* p) {
    fn_80021594(p, 1, 0x5D705BF5);
    *(void**)(p + 4) = lbl_80357528;
    *(s32*)(p + 0x90) = 0;
    *(u8*)(p + 0x94) = 0;
    *(s32*)(p + 0x98) = 0;
    *(s32*)(p + 0xA0) = 0;
    *(s32*)(p + 0xA4) = 0;
    *(s32*)(p + 0xA8) = 0;
    *(u8*)(p + 0xAC) = 0;
    *(f32*)(p + 0xB0) = 0.0f;
    *(s32*)(p + 0xB4) = 0;
    *(f32*)(p + 0xB8) = 0.0f;
    *(f32*)(p + 0xBC) = 0.0f;
    *(s32*)(p + 0xEC) = 0;
    *(s32*)(p + 0xF0) = 0;
    *(s32*)(p + 0xF4) = 0;
    *(s32*)(p + 0xF8) = 0;
    *(u8*)(p + 0xFC) = 0;
    *(u8*)(p + 0xFD) = 0;
    *(f32*)(p + 0x100) = -1.0f;
    *(s32*)(p + 0x104) = 0;
    *(s32*)(p + 0x114) = 0;
    *(s32*)(p + 0x118) = 0;
    *(s32*)(p + 0x11C) = 0;
    *(s32*)(p + 0x120) = 0;
    lbl_805FAFC0 = p;
    lbl_805FAFA0 = 0xA750E8D9;
    *(s32*)(p + 0xC0) = 0x1E;
    *(s32*)(p + 0xC4) = 3;
    *(s32*)(p + 0xC8) = 0xA;
    *(s32*)(p + 0xCC) = 0xD;
    *(f32*)(p + 0xD0) = 30.0f;
    *(s32*)(p + 0xD4) = 1;
    *(s32*)(p + 0xD8) = 0x14;
    {
        u8* rec = *(u8**)(gWorldState + 0x1A4) + (gZoneResourceSize * 2 - gZoneResourceSize);
        for (s32 i = 0; i < *(s32*)(gWorldState + 4); i++) {
            for (s32 j = 0; j < *(s32*)(rec + 0x98); j++) {
                u8* obj = (*(u8***)(rec + 0x12C))[j];
                u8* nm = *(u8**)(obj + 0x9C);
                fn_801EB708(nm != NULL ? *(u32*)(nm + 8) : 0);
                nm = *(u8**)(obj + 0x9C);
                u32 h = nm != NULL ? *(u32*)(nm + 8) : 0;
                if (h == 0xD716DA31u) *(u8**)(p + 0x90) = obj;
            }
        }
    }
    s32 n1 = *(s32*)(p + 0xC0);
    *(void**)(p + 0xA4) = fn_8032F028(MemAllocSmall(n1 * 0x128 + 0x10), (void*)fn_8002B4D0, (void*)fn_8002B514, 0x128, n1);
    s32 n2 = *(s32*)(p + 0xC4);
    *(void**)(p + 0xA8) = fn_8032F028(MemAllocSmall(n2 * 0x12C + 0x10), (void*)fn_8002AB00, (void*)fn_8002AB4C, 0x12C, n2);
    void* h = fn_801D40E0(0xC);
    if (h != NULL) fn_8003107C((u8*)h);
    *(void**)(p + 0xF8) = h;
    fn_80075264(p, 0x3C5);
    *(s32*)(p + 8) = 0x2A5;
    *(s32*)(p + 0xC) = 0x2A6;
    *(u32*)(p + 0x30) = 0xC76D213Eu;
    *(u32*)(p + 0x34) = 0x5E647084u;
    return p;
}

// 0x8002C83C
// Constructor of a controller object (hash 0xE718CB83): clears a table of 5 entries (stride 0x28),
// finds the zone object with name hash 0x954D0F3A, copies two float vectors and sets defaults.
u8* fn_8002C83C(u8* p) {
    fn_80021594(p, 8, 0xE718CB83);
    *(s32*)(p + 0x9C) = 0;
    *(void**)(p + 4) = lbl_80357880;
    *(u8*)(p + 0xA0) = 0;
    *(f32*)(p + 0xA4) = 0.0f;
    *(s32*)(p + 0xA8) = 4;
    *(f32*)(p + 0xAC) = 0.0f;
    *(s32*)(p + 0xB0) = 0;
    *(s32*)(p + 0xB4) = 0;
    *(u8*)(p + 0xF0) = 0;
    *(s32*)(p + 0xF4) = 0;
    for (u8* e = p + 0xF8; e < p + 0x1C0; e += 0x28) {
        *(s32*)(e + 0xC) = 0;
        *(f32*)(e + 0x14) = 0.0f;
        *(f32*)(e + 0x10) = 0.0f;
        *(s32*)(e + 0x1C) = 0;
        *(s32*)(e + 0x20) = 0;
        *(s32*)(e + 0x24) = 0;
        *(u8*)(e + 0x19) = 0;
    }
    fn_801F9D3C(p + 0x21C, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F9544(p + 0x31C, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(p + 0x3D4) = 0;
    *(f32*)(p + 0x3D0) = 1.0f;
    lbl_805FAFD8 = p;
    u8* found = NULL;
    *(s32*)(p + 0x94) = -1;
    *(s32*)(p + 0x98) = -1;
    *(s32*)(p + 0x90) = 0;
    lbl_805FAFA0 = 0x49D29DC3;
    *(f32*)(p + 0xBC) = 15.0f;
    *(f32*)(p + 0xB8) = 3.0f;
    *(f32*)(p + 0xC0) = 1.5f;
    FindZoneObj(&found, 0x954D0F3Au);
    *(u8**)(p + 0x9C) = found;
    *(s32*)(p + 0x1C0) = 0;
    *(s32*)(p + 0x1C4) = 0;
    *(s32*)(p + 0x1C8) = 0;
    *(s32*)(p + 0x1CC) = 0;
    *(s32*)(p + 0x1D0) = 0;
    *(s32*)(p + 0x1D4) = 0;
    *(s32*)(p + 0x1D8) = 0;
    *(s32*)(p + 0x1DC) = 0;
    for (s32 o = 0x1E0; o <= 0x218; o += 4) *(f32*)(p + o) = 0.0f;
    *(f32*)(p + 0xC4) = lbl_80347528[0];
    *(f32*)(p + 0xD4) = lbl_80347538[0];
    *(f32*)(p + 0xC8) = lbl_80347528[1];
    *(f32*)(p + 0xD8) = lbl_80347538[1];
    *(f32*)(p + 0xCC) = lbl_80347528[2];
    *(f32*)(p + 0xDC) = lbl_80347538[2];
    *(f32*)(p + 0xD0) = lbl_80347528[3];
    *(f32*)(p + 0xE0) = lbl_80347538[3];
    *(s32*)(p + 0xE4) = 1;
    *(s32*)(p + 0xE8) = 4;
    *(f32*)(p + 0xEC) = 0.008333334f;
    *(s32*)(p + 0x2B4) = 0;
    fn_801B3310(fn_801B3308(), p + 0x21C);
    *(s32*)(p + 0x3B4) = 1;
    *(f32*)(p + 0x3B0) = 0.1f;
    fn_801B3310(fn_801B3308(), p + 0x31C);
    fn_80075264(p, 0x3CD);
    *(s32*)(p + 8) = 0x3DF;
    *(s32*)(p + 0xC) = 0x3E0;
    *(u32*)(p + 0x30) = 0x2EDE86EEu;
    *(u32*)(p + 0x34) = 0xB7D7D754u;
    lbl_80357748[0] = 0.1f;
    lbl_80357748[1] = 0.5f;
    lbl_80357748[2] = 0.3f;
    lbl_80357748[3] = 1.0f;
    lbl_80357748[4] = 0.9f;
    return p;
}

// 0x8002DE9C
// Constructor of a controller object (hash 0xEC8941AD): sets up sub-objects and five 10-element
// arrays, replaces the global instance, and rolls two random timing values (0xF58 / 0xF5C).
u8* fn_8002DE9C(u8* p) {
    fn_80021594(p, 6, 0xEC8941AD);
    *(void**)(p + 4) = lbl_803579D8;
    fn_80030228(p + 0x90);
    fn_80030228(p + 0xC8);
    fn_801F61AC(p + 0x100, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F9C2C(p + 0x1BC, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(p + 0x278) = 0;
    *(s32*)(p + 0x27C) = 0;
    *(s32*)(p + 0x294) = 0;
    *(s32*)(p + 0x298) = 0;
    *(s32*)(p + 0x29C) = 0;
    *(s32*)(p + 0x2A0) = 0;
    *(s32*)(p + 0x2A4) = 0;
    *(s32*)(p + 0x2A8) = 0;
    fn_8032F1E8(p + 0x2B4, (void*)fn_80030B6C, (void*)fn_80030BC8, 0x44, 0xA);
    fn_8032F1E8(p + 0x55C, (void*)fn_80030604, (void*)fn_80030648, 0x38, 0xA);
    fn_8032F1E8(p + 0x78C, (void*)fn_800308A4, (void*)fn_800308E8, 0x38, 0xA);
    fn_8032F1E8(p + 0x9BC, (void*)fn_80030754, (void*)fn_80030798, 0x38, 0xA);
    fn_8032F1E8(p + 0xBEC, (void*)fn_800309F4, (void*)fn_80030A40, 0x3C, 0xA);
    *(s32*)(p + 0xF0C) = 0;
    *(f32*)(p + 0xF3C) = 0.0f;
    *(f32*)(p + 0xF40) = 0.0f;
    *(f32*)(p + 0xF44) = 0.0f;
    *(f32*)(p + 0xF48) = 0.0f;
    *(f32*)(p + 0xF4C) = 0.0f;
    *(f32*)(p + 0xF50) = 0.0f;
    *(f32*)(p + 0xF54) = 0.0f;
    *(s32*)(p + 0xF60) = 0;
    *(s32*)(p + 0xF64) = 0;
    *(s32*)(p + 0xF68) = 0;
    *(s32*)(p + 0xF6C) = 0;
    *(u8*)(p + 0xF74) = 1;
    *(u8*)(p + 0xF7C) = 0;
    *(s32*)(p + 0xF80) = 0;
    *(s32*)(p + 0xF84) = 0;
    if (lbl_805FAFE0 != NULL) {
        void* o = lbl_805FAFE0;
        (*(void (**)(void*, s32))(*(u8**)((u8*)o + 4) + 8))(o, 1);
    }
    lbl_805FAFE0 = p;
    lbl_805FAFA0 = 0x48012811;
    fn_801B3310(fn_801B3308(), p + 0x100);
    *(s32*)(p + 0x254) = 1;
    *(f32*)(p + 0x270) = 1.0f;
    fn_801B3310(fn_801B3308(), p + 0x1BC);
    f32 hiA = lbl_805FC5E8[1];
    *(f32*)(p + 0xF58) = lbl_805FC5E8[0] + (hiA - lbl_805FC5E8[0]) * (2.3283064e-10f * (f32)RandomU32());
    f32 hiB = lbl_805FC5F0[1];
    u32 r2 = RandomU32();
    *(f32*)(p + 0xF58) = *(f32*)(p + 0xF58) / 2.5f;
    *(s32*)(p + 0xF70) = 0x18;
    *(f32*)(p + 0xF5C) = (lbl_805FC5F0[0] + (hiB - lbl_805FC5F0[0]) * (2.3283064e-10f * (f32)r2)) * 0.25f;
    memset(p + 0xE6C, 0, 4);
    memset(p + 0xEBC, 0, 4);
    memset(p + 0xE94, 0, 4);
    memset(p + 0xE44, 0, 4);
    memset(p + 0xEE4, 0, 4);
    memset(p + 0x284, 0, 8);
    memset(p + 0x28C, 0, 8);
    memset(p + 0x284, 0, 8);
    memset(p + 0x28C, 0, 8);
    *(f32*)(p + 0x2AC) = 400.0f;
    *(f32*)(p + 0x2B0) = 599.9998f;
    *(s32*)(p + 0xF78) = 0;
    *(f32*)(p + 0xF38) = 0.5f;
    memset(p + 0xF10, 0, 0x28);
    fn_80075264(p, 0x3D6);
    *(s32*)(p + 8) = 0x1010;
    *(s32*)(p + 0xC) = 0x1011;
    *(u32*)(p + 0x30) = 0xBFF1B613u;
    *(u32*)(p + 0x34) = 0x26F8E7A9u;
    return p;
}

// 0x800314F8
// Constructor of a controller object (hash 0x2FEE64A3): clears state, finds the zone object with
// name hash 0xBABF4579 and allocates an array of 0x60-byte elements (count at +0xA8).
u8* fn_800314F8(u8* p) {
    fn_80021594(p, 3, 0x2FEE64A3);
    *(s32*)(p + 0x90) = 0;
    *(void**)(p + 4) = lbl_80357C60;
    *(u8*)(p + 0x94) = 0;
    *(s32*)(p + 0xAC) = 0;
    *(f32*)(p + 0xB0) = 0.0f;
    *(f32*)(p + 0xB4) = 0.0f;
    *(f32*)(p + 0xB8) = 0.0f;
    *(f32*)(p + 0xBC) = 0.0f;
    *(s32*)(p + 0xC0) = 0;
    *(u8*)(p + 0xC4) = 0;
    *(s32*)(p + 0xC8) = 0;
    *(s32*)(p + 0xCC) = 0;
    *(s32*)(p + 0xD0) = 0;
    *(f32*)(p + 0xD4) = 0.0f;
    *(f32*)(p + 0xD8) = 0.0f;
    *(s32*)(p + 0xDC) = 0;
    *(s32*)(p + 0xE0) = 0;
    *(s32*)(p + 0xE4) = 0;
    lbl_805FAFE8 = p;
    lbl_805FAFA0 = 0x6D0FCFD5;
    *(s32*)(p + 0x98) = 0x1B;
    *(s32*)(p + 0x9C) = 0x28;
    *(s32*)(p + 0xA0) = 1;
    *(s32*)(p + 0xA4) = 1;
    *(s32*)(p + 0xA8) = 0x1B;
    FindZoneObj((u8**)(p + 0x90), 0xBABF4579u);
    s32 n = *(s32*)(p + 0xA8);
    *(void**)(p + 0xC0) = fn_8032F028(MemAllocSmall(n * 0x60 + 0x10), (void*)fn_80032EDC, (void*)fn_80032F7C, 0x60, n);
    fn_80075264(p, 0x3D1);
    *(s32*)(p + 8) = 0x692;
    *(s32*)(p + 0xC) = 0x693;
    *(u32*)(p + 0x30) = 0x982A752Fu;
    *(u32*)(p + 0x34) = 0x01232495u;
    return p;
}

// 0x80034084
// Constructor of a UI controller object (hash 0x8FA001CA): sets the layout constants (screen
// positions/sizes as floats) and clears state.
u8* fn_80034084(u8* p) {
    fn_80021594(p, 7, 0x8FA001CA);
    *(void**)(p + 4) = lbl_80357E80;
    fn_801F9C2C(p + 0x90, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(p + 0x160) = 0;
    *(f32*)(p + 0x164) = 256.0f;
    *(u8*)(p + 0x168) = 0;
    *(f32*)(p + 0x16C) = 0.0f;
    *(f32*)(p + 0x170) = 10.0f;
    *(f32*)(p + 0x174) = 0.125f;
    *(f32*)(p + 0x178) = 240.0f;
    *(s32*)(p + 0x17C) = 2;
    *(f32*)(p + 0x180) = 0.0f;
    *(f32*)(p + 0x184) = 0.125f;
    *(f32*)(p + 0x188) = 4.0f;
    *(s32*)(p + 0x250) = 0;
    *(s32*)(p + 0x254) = 0;
    *(s32*)(p + 0x360) = 0;
    *(s32*)(p + 0x364) = 0;
    *(s32*)(p + 0x420) = 0;
    *(s32*)(p + 0x424) = 0;
    *(s32*)(p + 0x4E0) = 0;
    *(s32*)(p + 0x4E4) = 0;
    *(f32*)(p + 0x588) = 114.0f;
    *(f32*)(p + 0x58C) = 325.0f;
    *(f32*)(p + 0x590) = 15.0f;
    *(f32*)(p + 0x594) = 7.5f;
    *(f32*)(p + 0x598) = 0.0f;
    *(s32*)(p + 0x59C) = 0;
    *(u8*)(p + 0x5AC) = 0;
    *(f32*)(p + 0x5B0) = 0.5f;
    *(f32*)(p + 0x5B4) = 0.0f;
    *(f32*)(p + 0x5B8) = 45.0f;
    *(f32*)(p + 0x5BC) = 110.0f;
    *(f32*)(p + 0x5C0) = 20.0f;
    *(f32*)(p + 0x5C4) = 6.0f;
    *(f32*)(p + 0x5C8) = 410.0f;
    *(f32*)(p + 0x5CC) = 317.0f;
    *(f32*)(p + 0x5D0) = 344.0f;
    *(f32*)(p + 0x5D4) = 40.0f;
    *(f32*)(p + 0x5D8) = 54.0f;
    *(f32*)(p + 0x5DC) = 50.0f;
    *(f32*)(p + 0x5E0) = 128.0f;
    *(f32*)(p + 0x5E4) = 73.0f;
    *(f32*)(p + 0x5E8) = 1.0f;
    *(f32*)(p + 0x5EC) = 49.0f;
    *(f32*)(p + 0x5F0) = 47.5f;
    *(s32*)(p + 0x5F4) = 0xF;
    *(f32*)(p + 0x5F8) = 0.0f;
    *(f32*)(p + 0x5FC) = 0.0f;
    *(f32*)(p + 0x600) = 0.0f;
    *(s32*)(p + 0x604) = 0;
    *(s32*)(p + 0x608) = 0;
    *(s32*)(p + 0x60C) = 0;
    *(s32*)(p + 0x610) = 0;
    *(s32*)(p + 0x614) = 0;
    *(s32*)(p + 0x618) = 0;
    *(f32*)(p + 0x61C) = 0.0f;
    *(s32*)(p + 0x620) = 1;
    *(s32*)(p + 0x624) = 0;
    *(f32*)(p + 0x62C) = -1.0f;
    *(s32*)(p + 0x630) = -1;
    *(u8*)(p + 0x634) = 0;
    *(s32*)(p + 0x638) = 0;
    *(s32*)(p + 0x63C) = 0;
    *(f32*)(p + 0x640) = 0.0f;
    *(u8*)(p + 0x644) = 0;
    lbl_805FAFA0 = 0x92AE284A;
    fn_80075264(p, 0x3DC);
    *(s32*)(p + 8) = 0x1088;
    *(s32*)(p + 0xC) = 0x1089;
    *(u32*)(p + 0x30) = 0xCE0AC702u;
    *(u32*)(p + 0x34) = 0x570396B8u;
    memset(p + 0x258, 0, 0x28);
    memset(p + 0x280, 0, 0x28);
    return p;
}

// 0x80037324
// Constructor of a variant-selecting controller object (hash 0x8F13FF01): picks a variant index
// (from gLab+0x64, or from the state at lbl_805FBBE0+0x108, random for -1/43), sets the matching
// global hash lbl_805FAF9C, and finishes via fn_80037FB4.
u8* fn_80037324(u8* p) {
    fn_80021594(p, 9, 0x8F13FF01);
    *(void**)(p + 4) = lbl_80357F58;
    fn_801F61AC(p + 0x90, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(p + 0x14C) = 0;
    *(s32*)(p + 0x150) = 0;
    *(s32*)(p + 0x154) = 0;
    *(s32*)(p + 0x158) = 0;
    *(s32*)(p + 0x15C) = 0;
    *(s32*)(p + 0x160) = 0;
    *(s32*)(p + 0x16C) = 0;
    *(f32*)(p + 0x170) = -1000.0f;
    *(s32*)(p + 0x9A4) = 1;
    *(s32*)(p + 0x9A8) = -1;
    *(f32*)(p + 0x9AC) = 0.0f;
    *(s32*)(p + 0x9B0) = 0;
    *(s32*)(p + 0xA44) = 0;
    *(s32*)(p + 0xA48) = 0;
    *(s32*)(p + 0xA54) = 0;
    *(s32*)(p + 0xA58) = 0;
    *(s32*)(p + 0xA5C) = -1;
    *(s32*)(p + 0xA60) = -1;
    *(s32*)(p + 0xA64) = 0;
    *(f32*)(p + 0xA68) = 0.0f;
    *(f32*)(p + 0xA6C) = 0.0f;
    *(f32*)(p + 0xA70) = 0.0f;
    *(f32*)(p + 0xA74) = 0.0f;
    lbl_805FAFA0 = 0x6E59E316;
    fn_801B3310(fn_801B3308(), p + 0x90);
    fn_80075264(p, 0x3DA);
    *(s32*)(p + 8) = 0x101C;
    *(s32*)(p + 0xC) = 0x101D;
    *(u32*)(p + 0x30) = 0x8BE1ED07u;
    *(u32*)(p + 0x34) = 0x12E8BCBDu;
    *(s32*)(p + 0x9A8) = *(s32*)((u8*)gLab + 0x64);
    if (*(s32*)(p + 0x9A8) == -1) {
        *(s32*)(p + 0x9A8) = -1;
    }
    if (*(s32*)(p + 0x9A8) == -1) {
        s32 v;
        switch (*(s32*)(lbl_805FBBE0 + 0x108)) {
        case -1:
        case 0x2B:
            v = (s32)(RandomU32() % 5);
            break;
        case 5:
            v = 2;
            break;
        case 1:
            v = 1;
            break;
        default:
            v = 0;
            break;
        }
        *(s32*)(p + 0x9A8) = v;
    }
    s32 idx = *(s32*)(p + 0x9A8);
    if (idx == 1) {
        lbl_805FAF9C = 0x7A27B25C;
    } else if (idx > 1 && idx < 5) {
        lbl_805FAF9C = 0x7ED50A98;
    } else {
        lbl_805FAF9C = 0x782022D6;
    }
    idx = *(s32*)(p + 0x9A8);
    *(s32*)(p + 0x164) = idx;
    *(s32*)(p + 0x168) = lbl_80347620[idx];
    fn_80037FB4(p);  // r4/r5 unused in callee
    return p;
}

// 0x80038618
// Constructor of a controller object (hash 0x14AB0F17): replaces the global instance, sets up a
// downward direction vector (0,-1,0), four 0x28-byte slots each with a 0x10-byte buffer, and
// two squared-radius floats (35^2, 25^2).
u8* fn_80038618(u8* p) {
    fn_80021594(p, 4, 0x14AB0F17);
    *(void**)(p + 4) = lbl_80358050;
    *(s32*)(p + 0x90) = 0;
    *(s32*)(p + 0x94) = 0;
    *(s32*)(p + 0xA0) = 0;
    fn_801F61AC(p + 0xA4, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F94B0(p + 0x160, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(p + 0x5E4) = 0;
    *(s32*)(p + 0x5E8) = 0;
    fn_8032F1E8(p + 0x5EC, (void*)fn_800389A4, (void*)fn_800385A8, 0x28, 4);
    *(u8*)(p + 0x68C) = 0;
    *(s32*)(p + 0x6B4) = 0x5A;
    *(s32*)(p + 0x6B8) = 0;
    *(s32*)(p + 0x6BC) = 0;
    *(s32*)(p + 0x6C0) = 0;
    *(s32*)(p + 0x6C4) = 0;
    *(s32*)(p + 0x6C8) = 0;
    *(s32*)(p + 0x6CC) = 0;
    *(s32*)(p + 0x6D0) = 0;
    *(f32*)(p + 0x6D4) = 0.0f;
    *(s32*)(p + 0x6D8) = 0;
    *(u8*)(p + 0x6DC) = 0;
    *(u8*)(p + 0x6DD) = 0;
    *(f32*)(p + 0x6E0) = 0.0f;
    *(f32*)(p + 0x6E4) = 0.0f;
    if (lbl_805FB008 != NULL) {
        u8* o = lbl_805FB008;
        (*(void (**)(void*, s32))(*(u8**)(o + 4) + 8))(o, 1);
    }
    lbl_805FB008 = p;
    lbl_805FAFA0 = 0xDA5AD4E1;
    // The asm normalises the constant vector (0,-1,0); the result is the unit vector itself.
    f32 dir[3] = {0.0f, -1.0f, 0.0f};
    fn_801B3310(fn_801B3308(), p + 0xA4);
    *(s32*)(p + 0x1F8) = 1;
    fn_801F85E4(p + 0x160, (const u8*)dir, 1);
    *(f32*)(p + 0x1FC) = 0.0f;
    *(f32*)(p + 0x200) = -1.0f;
    *(f32*)(p + 0x204) = 0.0f;
    *(f32*)(p + 0x1F4) = 0.00001f;
    fn_801F8FA0(p + 0x160, 1e-7f);
    fn_801F8F34(p + 0x160, 0.000001f);
    fn_801B3310(fn_801B3308(), p + 0x160);
    *(s32*)(p + 0x98) = 0;
    *(s32*)(p + 0x9C) = 0;
    for (s32 i = 0; i < 4; i++) {
        u8* e = p + i * 0x28;
        *(s32*)(e + 0x5FC) = 0;
        void* buf = fn_8032F028(MemAllocSmall(0x20), (void*)fn_80038578, NULL, 0x10, 1);
        *(void**)(e + 0x5F8) = buf;
        memset(buf, 0, 0x10);
        *(s32*)(e + 0x5FC) = 0;
        *(s32*)(e + 0x600) = 0;
        *(f32*)(e + 0x604) = 640.0f;
        *(f32*)(e + 0x60C) = 0.0f;
        *(f32*)(e + 0x608) = 240.0f;
        *(f32*)(e + 0x610) = 0.0f;
        *(s32*)(p + i * 4 + 0x69C) = 0;
    }
    *(f32*)(p + 0x6AC) = 35.0f * 35.0f;
    *(f32*)(p + 0x6B0) = 25.0f * 25.0f;
    *(f32*)(p + 0x690) = 0.0f;
    *(f32*)(p + 0x694) = 0.0f;
    *(f32*)(p + 0x698) = 0.0f;
    fn_80075264(p, 0x3C3);
    *(s32*)(p + 8) = 0x295;
    *(s32*)(p + 0xC) = 0x296;
    *(u32*)(p + 0x30) = 0x920FF37Au;
    *(u32*)(p + 0x34) = 0x0B06A2C0u;
    return p;
}

// 0x80039CF8
// Constructor of a controller object (hash 0xB8221B4C): allocates a 7-element array of 0x34-byte
// entries, sets layout floats, clears state and registers two sub-objects.
u8* fn_80039CF8(u8* p) {
    fn_80021594(p, 5, 0xB8221B4C);
    *(void**)(p + 4) = lbl_80358218;
    fn_8032F1E8(p + 0x90, (void*)fn_8003CD7C, (void*)fn_8003CDE0, 0x34, 7);
    *(f32*)(p + 0x378) = 25.0f;
    *(f32*)(p + 0x37C) = 300.0f;
    *(f32*)(p + 0x380) = 35.0f;
    *(f32*)(p + 0x384) = 330.0f;
    *(f32*)(p + 0x388) = 1.5f;
    *(f32*)(p + 0x38C) = 0.0f;
    *(f32*)(p + 0x390) = 0.75f;
    *(f32*)(p + 0x394) = 0.8f;
    *(s32*)(p + 0x398) = 0;
    *(s32*)(p + 0x39C) = -1;
    *(s32*)(p + 0x3A0) = 0;
    *(s32*)(p + 0x3A4) = 0;
    *(s32*)(p + 0x3A8) = 3;
    *(f32*)(p + 0x3AC) = 0.5f;
    *(f32*)(p + 0x3B0) = 0.0f;
    *(s32*)(p + 0x3B4) = 0;
    *(s32*)(p + 0x3B8) = 0;
    *(s32*)(p + 0x3D8) = 0;
    *(s32*)(p + 0x3DC) = -1;
    *(s32*)(p + 0x3E0) = -1;
    fn_801F8110(p + 0x3E4, *(s32*)(lbl_803C4298 + 0x38));
    fn_801F61AC(p + 0x868, *(s32*)(lbl_803C4298 + 0x38));
    *(s32*)(p + 0x924) = 0;
    *(s32*)(p + 0x928) = 0;
    *(s32*)(p + 0x92C) = 0;
    *(s32*)(p + 0x930) = 0;
    *(s32*)(p + 0x934) = 0;
    *(s32*)(p + 0x938) = 0;
    *(s32*)(p + 0x944) = 0;
    *(s32*)(p + 0x948) = 0;
    *(s32*)(p + 0x94C) = 0;
    *(s32*)(p + 0x950) = 0;
    *(f32*)(p + 0x954) = 0.5f;
    *(s32*)(p + 0x95C) = 0;
    *(s32*)(p + 0x960) = 0;
    *(s32*)(p + 0x964) = 0;
    *(s32*)(p + 0x968) = 0;
    *(s32*)(p + 0x96C) = 0;
    *(s32*)(p + 0x970) = 0;
    *(s32*)(p + 0x974) = 0;
    *(s32*)(p + 0x97C) = 0;
    *(f32*)(p + 0x980) = 0.0f;
    *(s32*)(p + 0x984) = 0;
    *(u8*)(p + 0x988) = 0;
    *(s32*)(p + 0x98C) = 0;
    *(s32*)(p + 0x990) = 0;
    *(u8*)(p + 0x994) = 0;
    *(s32*)(p + 0x998) = 0;
    *(f32*)(p + 0x99C) = 0.0f;
    *(s32*)(p + 0x9AC) = 0;
    *(s32*)(p + 0x9B0) = 0;
    *(s32*)(p + 0x9B4) = 0;
    *(f32*)(p + 0x9B8) = 256.0f;
    *(f32*)(p + 0x9BC) = 0.0f;
    *(f32*)(p + 0x9C0) = 0.0f;
    *(f32*)(p + 0x9C4) = 0.0f;
    lbl_805FAFA0 = 0x68BAEC35;
    lbl_805FAFA4 = 0x2E7C2F6E;
    fn_801B3310(fn_801B3308(), p + 0x868);
    *(s32*)(p + 0x47C) = 1;
    *(f32*)(p + 0x480) = 0.0f;
    *(f32*)(p + 0x484) = -1.0f;
    *(f32*)(p + 0x488) = 0.0f;
    fn_801B3310(fn_801B3308(), p + 0x3E4);
    *(s32*)(p + 0x344) = -1;
    *(s32*)(p + 0x340) = -1;
    *(s32*)(p + 0x33C) = -1;
    *(s32*)(p + 0x338) = -1;
    *(s32*)(p + 0x334) = -1;
    *(s32*)(p + 0x330) = -1;
    *(s32*)(p + 0x25C) = 0;
    *(s32*)(p + 0x260) = 0;
    *(s32*)(p + 0x264) = 0;
    *(s32*)(p + 0x268) = 0;
    *(s32*)(p + 0x26C) = 0;
    *(s32*)(p + 0x270) = 0;
    *(s32*)(p + 0x274) = 0;
    *(s32*)(p + 0x278) = 0;
    fn_80075264(p, 0x3C2);
    *(s32*)(p + 8) = 0x153;
    *(s32*)(p + 0xC) = 0x154;
    *(u32*)(p + 0x30) = 0xEF56F69Fu;
    *(u32*)(p + 0x34) = 0x765FA725u;
    return p;
}

// 0x8003E0A4
// Re-applies the two stored fighters (lbl_805FB01C / lbl_805FB020) to the battle when the level
// state is 0x1C: logs a broken torso slot, repairs its HP, creates fresh fighter owner objects
// (0x5C bytes), then clears the pending list at lbl_803C4E28 and the counters.
void fn_8003E0A4(void) {
    u8* sub = *(u8**)(lbl_805FB01C + 0x1C);
    if (sub != NULL) sub -= 0xC48;
    fn_8004DD30(sub);
    sub = *(u8**)(lbl_805FB020 + 0x1C);
    if (sub != NULL) sub -= 0xC48;
    fn_8004DD30(sub);
    if (gBattle != NULL && lbl_803C4298[0x1C] == 0x1C) {
        BattleSetFighter(gBattle, 0, lbl_805FB01C, 0);
        BattleSetFighter(gBattle, 1, lbl_805FB020, 0);
        u8** slot = FighterGetSlot(lbl_805FB020, 4);
        if (slot == NULL) {
            ZoneLog(gIdPool, "Torso Missing combat Info!!!!\n");
        } else if (*(s32*)*slot <= 0) {
            ZoneLog(gIdPool, "Torso combat Info HP < 0!!!!\n");
            ZoneLog(gIdPool, "Part HP %d\n", *(s32*)(*slot + 0x0));
            ZoneLog(gIdPool, "Part Quality %d\n", *(s32*)(*slot + 0x10));
            ZoneLog(gIdPool, "Part Enhancement %x\n", *(s32*)(*slot + 0x20));
            ZoneLog(gIdPool, "Part Defect %x\n", *(s32*)(*slot + 0x24));
            ZoneLog(gIdPool, "Part KnockedOff %d\n", *(s32*)(*slot + 0x28));
            *(s32*)*slot = *(s32*)(*slot + 4);
        }
        u8* owner = (u8*)fn_801D40E0(0x5C);
        if (owner != NULL) owner = fn_800496DC(owner, (s32)(intptr_t)lbl_805FB01C);  // lint:ok 32-bit slot
        FighterStatsSetOwner(lbl_805FB01C + 0x20, owner);
        owner = (u8*)fn_801D40E0(0x5C);
        if (owner != NULL) owner = fn_800496DC(owner, (s32)(intptr_t)lbl_805FB020);  // lint:ok 32-bit slot
        FighterStatsSetOwner(lbl_805FB020 + 0x20, owner);
    }
    lbl_805FB024 = 1;
    for (s32 i = 0; i < lbl_805FB038; i++) lbl_803C4E28[i] = 0;
    lbl_805FB034 = 0;
    lbl_805FB038 = 0;
}

// 0x801CA30C
// Builds an orthonormal basis from the direction vector in arg1 (x,y,z,w; normalised, with an
// up vector of +Z, or +Y when the direction is parallel to it) and uses it to rotate the local
// offset stored at (obj+0x5C)+0x1BC; the result is written as a position to arg0+0x30 and +0x40.
void fn_801CA30C(u8* arg0, f32* arg1) {
    f32 x = arg1[0];
    f32 y = arg1[1];
    f32 z = arg1[2];
    f32 len2 = z * z + (x * x + y * y);
    f32 ux = 0.0f;
    f32 uy = 0.0f;
    f32 uz = 1.0f;
    f32 dx, dy, dz;
    if (len2 == 0.0f) {
        dx = 0.0f;
        dy = 0.0f;
        dz = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        dx = x * inv;
        dy = y * inv;
        dz = z * inv;
    }
    f32 t = fabsf(uz * dz + (ux * dx + uy * dy)) - 1.0f;
    if (t > -0.0001f && t < 0.0001f) {
        ux = 0.0f;
        uy = 1.0f;
        uz = 0.0f;
    }
    u8* o = *(u8**)(arg0 + 0x5C);
    f32 vy = *(f32*)(o + 0x1C0);
    f32 vx = *(f32*)(o + 0x1BC);
    f32 vz = *(f32*)(o + 0x1C4);
    f32 A = dy * uz - dz * uy;
    f32 B = dz * ux - dx * uz;
    f32 C = dx * uy - dy * ux;
    f32 D = B * dz - C * dy;
    f32 E = C * dx - A * dz;
    f32 F = A * dy - B * dx;
    f32 out0 = 0.0f + (D * vz + (A * vx + dx * vy));
    f32 out1 = 0.0f + (E * vz + (B * vx + dy * vy));
    f32 out2 = 0.0f + (F * vz + (C * vx + dz * vy));
    *(f32*)(arg0 + 0x40) = out0;
    *(f32*)(arg0 + 0x44) = out1;
    *(f32*)(arg0 + 0x48) = out2;
    *(f32*)(arg0 + 0x4C) = 0.0f;
    *(f32*)(arg0 + 0x30) = out0;
    *(f32*)(arg0 + 0x34) = out1;
    *(f32*)(arg0 + 0x38) = out2;
    *(f32*)(arg0 + 0x3C) = 0.0f;
}

}  // extern "C"
