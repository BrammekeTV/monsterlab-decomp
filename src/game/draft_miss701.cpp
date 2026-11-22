// Functions in 0x8002xxxx..0x8004xxxx: marker/screen-element setup, sewer stream setup,
// battle-screen update, selection HUD update and battle roster clear.
#include "types.h"

extern "C++" void* gScreenSets;                  // 0x805FB654

typedef void (*FnObjS32)(void* self, s32 a);
typedef s32 (*FnObjRetS32)(void* self);

extern "C" {

void*  ChoiceDestroy(void* c, s32 del);                          // 0x8020F394
void   ChoiceInit(u8* c);
void   MonsterCopyName(void* dst, void* src);                    // 0x8020EBB8
s32    PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);   // 0x80209E70
void*  ScreenSetFindElement(void* set, u32 hash);                // 0x80198F18
void   ScreenSetsActivate(void* sets, u32 hash);                 // 0x8018D70C
void*  ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
u32    StringHash(const char* s);
int    SysSprintf(char* buf, const char* fmt, ...);
void   UISetText(void* el, const char* text, s32 a);             // 0x8019EFC8
void   UIElement_SetVisibleFlag(u32 hash, BOOL on);
void   GameFlowSignal(s32 id, s32 a, s32 b);                     // 0x8000C7F4
void   StopRumble(s32 chan);                                     // small_801Bb.cpp
void   HideSelectionHUD(void);
void   ShowSelectionHUD(void);

void   fn_800222F0(void);                                        // 0x800222F0
void   fn_80021ECC(void);                                        // 0x80021ECC
void   fn_800223F0(u8* p, f32 v);                                // 0x800223F0
void   fn_800250EC(u8* p);
void   fn_80024958(u8* p);
void   fn_800311E8(u8* box, u8* pt);
void   fn_801A8654(u8* obj, s32 a1);                             // 0x801A8654
void   fn_801A8E80(u8* obj, s32 flag);
void   fn_801BBB68(void* mgr, s32 id, s32 a, s32 b);
s32    fn_8019BBA4(u8* a, u8* b, s32 c);
void   fn_80038248(u8* p);
void   fn_8020A398(s32 handle);                                  // 0x8020A398
void*  fn_801D40E0(s32 n);                                       // 0x801D40E0
void*  fn_80209814(void* v);                                     // 0x80209814
void   fn_8020F46C(u8* p, u8* choice);                           // 0x8020F46C
void   fn_8004DDEC(u8* p);                                  // draft_miss511 defines it as void
s32    fn_80040550(void);
void   fn_8004012C(s32 flags);
u8     fn_80046F54(void);                                        // 0x80046F54 (leaf, returns u8)
void   fn_80046F30(u8* out);
u8     fn_80049364(void);                                        // 0x80049364 (leaf, returns u8)
u32    fn_8018DEE0(u8* a3);                                      // 0x8018DEE0 (leaf, returns u32)

extern u8   gGame[];                                             // 0x803B53A0
extern u8*  gBattle;                                             // 0x805FBBB8
extern u8*  lbl_805FAFD8;
extern u8   lbl_803C4298[];
extern s32  lbl_803C4E28[];
extern s32  lbl_805FB034;
extern s32  lbl_805FB038;
extern u8   lbl_805FB024;
extern u8   lbl_805FB025;
extern s32  lbl_805FB028;
extern u8   lbl_805FB0E0;
extern s32  lbl_805FB970;
extern s32  lbl_805FB974;
extern void* lbl_805FB964;
extern void* lbl_805FBBAC;
extern f32  lbl_805FCBB0;
extern f32  lbl_805FCC20;
extern f32  lbl_805FCC88;
extern f32  lbl_805FCEBC;
extern f32  lbl_805FD0F8;
extern f32  lbl_805FD0FC;
extern const char lbl_805F8498[];   // format string
extern const char lbl_80355A28[];   // format "Marker%c_%d"
extern const char lbl_80355A34[];   // format
extern const char lbl_80355A40[];   // format "digging_treasure_unveiled_FX_%d"
extern const char lbl_80355A60[];   // hash key string
extern const char lbl_80357964[];   // format
extern const char lbl_80357978[];   // format

// 0x80023D74
// Rebuilds the marker element pointers of the screen (3 groups of Marker%c_%d, then 10 MarkerT_%d).
void fn_80023D74(u8* arg0) {
    fn_800222F0();
    void* sets = ScreenSetsCurrent(gScreenSets);
    // The 12-byte stack copy of lbl_803476F0 made by the original is dead (the buffer is
    // overwritten by SysSprintf), so it is not reproduced.
    s32 off = 0;
    for (s32 g = 0; g < 4; g++) {
        s32 count = (g == 2) ? 7 : 6;
        for (s32 k = 0; k < count; k++) {
            char buf[0x20];
            SysSprintf(buf, lbl_80355A28, 0x41 + g, k);
            void* el = ScreenSetFindElement(sets, StringHash(buf));
            fn_800311E8(*(u8**)(arg0 + 0xADC) + off, (u8*)el + 0x68);
        }
        off += 0xC;
    }
    for (s32 k = 0; k < 10; k++) {
        char buf[0x20];
        SysSprintf(buf, lbl_80355A34, k);
        void* el = ScreenSetFindElement(sets, StringHash(buf));
        fn_800311E8(arg0 + 0xAE0, (u8*)el + 0x68);
    }
}

// 0x80023F18
// Caches the screen elements used by the treasure-dig view and copies the dust cloud transform.
void fn_80023F18(u8* arg0) {
    fn_801BBB68(*(void**)(*(u8**)(*(u8**)(arg0 + 0xB8) + 0xA4)), (s32)0xF15918A6, 0, 0);
    void* sets = ScreenSetsCurrent(gScreenSets);
    // The 0x80-byte stack copy of lbl_80347700 is dead (SysSprintf overwrites the buffer) and is omitted.
    for (s32 i = 0; i < 3; i++) {
        char buf[0x20];
        SysSprintf(buf, lbl_80355A40, i + 1);
        // NOTE: packed 4-byte pointer slots (stride 4 in the asm).
        *(void**)(arg0 + 0xAC + i * 4) = ScreenSetFindElement(sets, StringHash(buf));
    }
    *(void**)(arg0 + 0x90) = ScreenSetFindElement(sets, 0x0A7964E0);
    *(void**)(arg0 + 0x100) = ScreenSetFindElement(sets, 0xA5EF21B9);
    *(void**)(arg0 + 0x98) = ScreenSetFindElement(sets, 0xDE8244C0);
    *(void**)(arg0 + 0x9C) = ScreenSetFindElement(sets, 0x478B157A);
    *(void**)(arg0 + 0x94) = *(void**)(arg0 + 0x98);
    *(void**)(arg0 + 0xA0) = ScreenSetFindElement(sets, 0x43040F35);
    *(void**)(arg0 + 0xA4) = ScreenSetFindElement(sets, 0xDA0D5E8F);
    *(void**)(arg0 + 0xA8) = ScreenSetFindElement(sets, 0xAD0A6E19);
    fn_80024958(arg0);
    u32 h = StringHash(lbl_80355A60);
    u8* el = (u8*)ScreenSetFindElement(ScreenSetsCurrent(gScreenSets), h);
    *(f32*)(arg0 + 0xC8) = *(f32*)(el + 0x80);
    *(f32*)(arg0 + 0xCC) = *(f32*)(el + 0x84);
    *(f32*)(arg0 + 0xD0) = *(f32*)(el + 0x88);
    *(f32*)(arg0 + 0xD4) = *(f32*)(el + 0x8C);
    *(f32*)(arg0 + 0xD8) = *(f32*)(el + 0x74);
    *(f32*)(arg0 + 0xDC) = *(f32*)(el + 0x78);
    *(f32*)(arg0 + 0xE0) = *(f32*)(el + 0x7C);
    fn_800250EC(arg0);
}

// 0x8002DA94
// Per-frame update of the 3 front and 5 back indicator objects; fades the timer f1 by farg0.
void fn_8002DA94(u8* arg0, f32 farg0) {
    f32 v = *(f32*)(lbl_805FAFD8 + 0xAC);
    if ((v < 0.0f ? -v : v) > lbl_805FCBB0) {
        for (s32 i = 0; i < 3; i++) {
            u8* obj = *(u8**)(arg0 + 0x1C0 + i * 4);
            ((FnObjS32)(*(void**)(*(u8**)(obj + 0x138) + 0x2C)))(obj, 0x1A);
            fn_801A8654(obj, 1);
        }
    } else {
        for (s32 i = 0; i < 3; i++) {
            u8* obj = *(u8**)(arg0 + 0x1C0 + i * 4);
            ((FnObjS32)(*(void**)(*(u8**)(obj + 0x138) + 0x30)))(obj, 2);
            fn_801A8654(obj, 0);
        }
    }
    f32 t = *(f32*)(arg0 + 0xC0);
    if (t > lbl_805FCBB0) {
        *(f32*)(arg0 + 0xC0) = t - farg0;
        for (s32 i = 0; i < 5; i++) {
            u8* obj = *(u8**)(arg0 + 0x1CC + i * 4);
            ((FnObjS32)(*(void**)(*(u8**)(obj + 0x138) + 0x2C)))(obj, 0x1A);
            fn_801A8654(obj, 1);
        }
    } else {
        for (s32 i = 0; i < 5; i++) {
            u8* obj = *(u8**)(arg0 + 0x1CC + i * 4);
            ((FnObjS32)(*(void**)(*(u8**)(obj + 0x138) + 0x30)))(obj, 0x1A);
            fn_801A8654(obj, 0);
        }
    }
}

// 0x8002F8B8
// Starts the sewer intro: plays the stream sound, caches the TopSewerStream elements and,
// on first use, hides the UI bar and snapshots the position into +0xF4C..+0xF54.
void fn_8002F8B8(u8* arg0) {
    fn_80021ECC();
    *(s32*)(arg0 + 0xF78) = PlaySound(0x4DF67A7D, 0, 0, 0, 1, 0);
    void* sets = ScreenSetsCurrent(gScreenSets);
    if (*(s32*)(arg0 + 0x284) == 0 || *(s32*)(arg0 + 0x28C) == 0) {
        for (s32 i = 0; i < 2; i++) {
            char buf[0x20];
            SysSprintf(buf, lbl_80357964, i);
            *(void**)(arg0 + 0x284 + i * 4) = ScreenSetFindElement(sets, StringHash(buf));
            SysSprintf(buf, lbl_80357978, i);
            *(void**)(arg0 + 0x28C + i * 4) = ScreenSetFindElement(sets, StringHash(buf));
        }
    }
    fn_800223F0(arg0, lbl_805FCC88);
    UIElement_SetVisibleFlag(0x0CAF9C6F, TRUE);
    *(void**)(arg0 + 0x280) = ScreenSetFindElement(sets, 0x65F41631);
    s32 fresh = 0;
    if (*(f32*)(arg0 + 0xF4C) == lbl_805FCC20 && *(f32*)(arg0 + 0xF50) == lbl_805FCC20 &&
        *(f32*)(arg0 + 0xF54) == lbl_805FCC20) {
        fresh = 1;
    }
    if (fresh) {
        u8* el = *(u8**)(arg0 + 0x280);
        *(f32*)(arg0 + 0xF4C) = *(f32*)(el + 0x68);
        *(f32*)(arg0 + 0xF50) = *(f32*)(el + 0x6C);
        *(f32*)(arg0 + 0xF54) = *(f32*)(el + 0x70);
        *(f32*)(arg0 + 0xF48) = *(f32*)(el + 0x74);
        *(f32*)(el + 0x74) = lbl_805FCC20;
        u8* obj = *(u8**)(arg0 + 0x280);
        ((FnObjS32)(*(void**)(*(u8**)(obj + 0x138) + 0x2C)))(obj, 0x18);
    }
}

// 0x800383A0
// Resets the battle-screen state and starts the matching sound or the UI text update.
void fn_800383A0(u8* p) {
    *(s32*)(p + 0x9B0) = 0;
    *(f32*)(p + 0x9AC) = lbl_805FCEBC;
    *(s32*)(p + 0x14C) = 0;
    u8* obj = *(u8**)(p + 0xA64);
    *(f32*)(obj + 0x74) = lbl_805FCEBC;
    ((FnObjS32)(*(void**)(*(u8**)(obj + 0x138) + 0x2C)))(obj, 0x1A);

    u8* o154 = *(u8**)(p + 0x154);
    fn_801A8E80(o154, 0);
    o154 = *(u8**)(p + 0x154);
    ((FnObjS32)(*(void**)(*(u8**)(o154 + 0x138) + 0x30)))(o154, 2);

    fn_8019BBA4(*(u8**)(p + 0x980), *(u8**)(p + 0x15C), 10);
    if (*(s32*)(p + 0x16C) == *(s32*)(p + 0x168)) {
        GameFlowSignal((s32)0xC608C842, 0, 0);
        StopRumble(*(s32*)(lbl_803C4298 + 0x38));
        if (*(s32*)(p + 0xA60) != -1) {
            fn_8020A398(*(s32*)(p + 0xA60));
            *(s32*)(p + 0xA60) = -1;
        }
        if (*(s32*)(p + 0xA5C) != -1) {
            fn_8020A398(*(s32*)(p + 0xA5C));
            *(s32*)(p + 0xA5C) = -1;
        }
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) v = fn_80209814(v);
            lbl_805FBBAC = v;
        }
        PlaySound(0x742575F9, 0, 0, 0, 0, 0);
    } else {
        fn_80038248(p);
        u8* o980 = *(u8**)(p + 0x980);
        ((FnObjS32)(*(void**)(*(u8**)(o980 + 0x138) + 0x2C)))(o980, 2);
        fn_8019BBA4(*(u8**)(p + 0x980), *(u8**)(p + 0x160), 10);
    }

    if (*(u8**)(p + 0x150) != NULL) {
        char buf[0x20];
        SysSprintf(buf, lbl_805F8498, *(s32*)(p + 0x16C), *(s32*)(p + 0x168));
        UISetText(*(void**)(p + 0x150), buf, 1);
        u8* o150 = *(u8**)(p + 0x150);
        ((FnObjS32)(*(void**)(*(u8**)(o150 + 0x138) + 0x2C)))(o150, 0x18);
    }
}

// 0x8003D7A8
// Copies the two active battle monsters' names into the roster, then zeroes lbl_803C4E28[0..count)
// (count = lbl_805FB038) and resets the roster state globals.
void fn_8003D7A8(void) {
    s32 idxA = *(s32*)(gGame + 0xD62C);
    s32 idxB = *(s32*)(gGame + 0xD630);
    if (idxA >= 0 && idxB >= 0) {
        u8* d = *(u8**)(*(u8**)(gBattle + 0xA00) + 0x1C);
        if (d != NULL) d -= 0xC48;
        fn_8004DDEC(d);
        MonsterCopyName((u8*)gGame + 0xAB98 + idxA * 0x13C, d + 0x898);

        u8* x = *(u8**)(gBattle + 0xA04);
        if (*(u8**)(gBattle + 0xA4C) != x) {
            d = *(u8**)(x + 0x1C);
            if (d != NULL) d -= 0xC48;
            fn_8004DDEC(d);
            MonsterCopyName((u8*)gGame + 0xAB98 + idxB * 0x13C, d + 0x898);
        }
    }
    s32 count = lbl_805FB038;
    for (s32 i = 0; i < count; i++) lbl_803C4E28[i] = 0;
    lbl_805FB038 = 0;
    lbl_805FB034 = 0;
    lbl_805FB024 = 0;
    lbl_805FB025 = 0;
    lbl_805FB028 = 0;
}

// 0x800403E4
// Per-frame selection HUD update: shows the HUD, or opens/closes a choice when the menu is active.
void fn_800403E4(u8* p) {
    *(f32*)(p + 0x4C) = lbl_805FD0F8;
    if (lbl_805FB0E0 != 0) return;
    if (fn_80049364() == 0) {
        ShowSelectionHUD();
        return;
    }
    fn_80040550(); /* r3 (p) is unused by the callee */
    fn_8004012C(0); // asm passes fn_80040550's leftover r3 (stale register)
    if (fn_8018DEE0((u8*)gScreenSets) == 0) {
        ScreenSetsActivate(gScreenSets, 0x7A1060A7);
    }
    if (fn_80046F54()) {
        *(f32*)(p + 0x4C) = lbl_805FD0F8;
        HideSelectionHUD();
        u8 choice[0xC];
        fn_80046F30(choice);
        fn_8020F46C(p, choice);
        ChoiceDestroy(choice, -1);
        return;
    }
    s32 active = 0;
    if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL) {
        s32 r = ((FnObjRetS32)(*(void**)(*(u8**)lbl_805FB964 + 0x34)))(lbl_805FB964);
        if (r != 0) active = 1;
    }
    if (active != 0 && *(f32*)(p + 0x4C) > lbl_805FD0FC) {
        *(u8*)(gBattle + 0x9FE) = 1;
        u8 choice[0xC];
        ChoiceInit(choice);
        *(s32*)(choice + 0) = 0x4E;
        *(u8*)(choice + 4) = 0;
        *(s32*)(choice + 8) = 4;
        *(f32*)(p + 0x4C) = lbl_805FD0F8;
        HideSelectionHUD();
        fn_8020F46C(p, choice);
        ChoiceDestroy(choice, -1);
    }
}

}  // extern "C"
