#include "types.h"

// Script callback prototypes (UIEvent_SendToAllScreens, SetPartSelected, TimedRumble, ...).
extern "C" {
#include "game/script_callbacks.h"
}

typedef s32 (*VFnI)(void* self, s32 a);          // vtable slot taking (self, int)
typedef s32 (*VFnIF)(void* self, s32 a, f32 f);  // vtable slot taking (self, int, float)
typedef s32 (*VFnIFF)(void* self, s32 a, f32 f1, f32 f2);  // vtable slot (self, int, float, float)

extern "C" {
u8*     fn_8020EAD4(u8* arg0);                   // 0x8020EAD4 (element ctor passed to fn_8032F1E8)
void    fn_8020EB44(u8* p, s32 del);             // 0x8020EB44 (element dtor passed to fn_8032F1E8)
void    fn_8032F1E8(u8* arr, u8* (*ctor)(u8*), void (*dtor)(u8*, s32), s32 size, s32 count);  // 0x8032F1E8
u8*     fn_80007A8C(u8* p, s32 del);             // 0x80007A8C (global object dtor)
void    fn_80052504(u8* p);                      // 0x80052504
u8*     SaveCtor(u8* s);                         // 0x80222228
u8*     fn_8021818C(u8* a);               // 0x8021818C
void    fn_800D7F24(u8* p);                      // 0x800D7F24
s32     fn_8004935C(void);                       // 0x8004935C
void*   fn_801D40E0(s32 n);                      // 0x801D40E0
void*   fn_80209814(void* v);                    // 0x80209814
void*   UIAnim_FindByName(void* set, u32 hash);        // 0x80198F1C
void    fn_80038248(u8* p);                      // 0x80038248
s32     fn_8019BBA4(u8* a, u8* b, s32 c);        // 0x8019BBA4
void    fn_800223F0(u8* p, f32 v);               // 0x800223F0
s32     PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);  // 0x80209E70
void*   ScreenSetFindElement(void* set, u32 hash);
void*   ScreenSetsCurrent(void* sets);
void    UISetText(void* el, const char* text, s32 a);
int     SysSprintf(char* buf, const char* fmt, ...);
void    __register_global_object(void* obj, void (*dtor)(void*), void* handle);
extern "C++" void* gScreenSets;
extern void* lbl_805FBBAC;                       // voice SFX handle
extern u32   lbl_805FAFA4;                       // screen element hash
extern u8    gGame[];
extern u8    lbl_803C29D8[];                     // 12-byte float vector
extern u8    lbl_803B5390[];                     // global object handle
extern f32   lbl_805FC600;                       // float constant (0.0)
extern f32   lbl_805FC604;                       // float constant (1.0)
extern f32   lbl_805FCEB8;                       // float constant (-1000.0)
extern f32   lbl_805FCEBC;                       // float constant (0.0)
extern f32   lbl_803475F8[];                     // float table indexed by state
extern const char lbl_805F8498[];                // format string "%d/%d"
void    fn_80021ECC(void);
void    fn_8002C63C(u8* p, u8 flag, u8* target, s32 arg3, f32 scale);  // 0x8002C63C (float in f1, after the integer registers)
u32     StringHash(const char* s);
extern u8  lbl_80357748[];                       // float table + format strings
extern u8  lbl_805F83E0[];                       // byte table (indices per element slot)
extern f32 lbl_805FCBB4;                         // float constant (1000.0)
}

// Creates the voice SFX handle lbl_805FBBAC once (fn_801D40E0, then fn_80209814 if that
// returned something).
static void EnsureVoiceSfxHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// Part-select feedback: select sound, then the two UI events (the second one is per part).
static void PartSelectFeedback(u32 partEvent) {
    EnsureVoiceSfxHandle();
    PlaySound(0x46712111u, 0, 0, 0, 0, 0);
    UIEvent_SendToAllScreens(0xD521D574u);
    UIEvent_SendToAllScreens(partEvent);
}

extern "C" {

// 0x80007814
// Init of the global game-state block gGame: clears the state words and the 256-entry
// table at +0x1C4, sets the global float vector, builds the two sub-object tables and
// registers the global object destructor.
void fn_80007814(void) {
    s32 i;
    u8* base = gGame + 0x5C8;

    *(u8*)(gGame + 0x13C) = 0;
    for (i = 0x140; i <= 0x1BC; i += 4) *(s32*)(gGame + i) = 0;
    *(f32*)(lbl_803C29D8 + 0) = lbl_805FC600;
    *(f32*)(lbl_803C29D8 + 4) = lbl_805FC604;
    *(f32*)(lbl_803C29D8 + 8) = lbl_805FC600;
    *(u8*)(gGame + 0x1C0) = 0;
    for (i = 0; i < 255; i++) *(s32*)(gGame + 0x1C4 + i * 4) = 0; // asm clears 255 entries (index u8-wrapped), the 256th is left alone

    for (u8* p = gGame + 0xA918; p < gGame + 0xAB48; p += 0x1C) fn_80052504(p);
    fn_8032F1E8(gGame + 0xAB98, fn_8020EAD4, fn_8020EB44, 0x13C, 0x14);
    SaveCtor(base + 0xBEA8);
    *(s32*)(base + 0xBE94) = 1;
    fn_8021818C(gGame + 0xD570); // asm also passes base+0x10000 in r4 (ignored by callee)
    fn_800D7F24(gGame + 0xD5F0);
    __register_global_object(gGame, (void (*)(void*))fn_80007A8C, lbl_803B5390);
}

// 0x8004012C
// Part-select input: when a direction bit of flags is held and that part is not the one
// already selected, selects it (SetPartSelected) and plays the feedback. flags == 0 with
// part 4 not selected selects part 4.
void fn_8004012C(s32 flags) {
    if ((flags & 1) && fn_8004935C() != 0) {
        if (SetPartSelected(0) != 0) PartSelectFeedback(0x91661B28u);
    } else if ((flags & 8) && fn_8004935C() != 3) {
        if (SetPartSelected(3) != 0) PartSelectFeedback(0x6CBB4FDDu);
    } else if ((flags & 4) && fn_8004935C() != 2) {
        if (SetPartSelected(2) != 0) PartSelectFeedback(0x23368BBCu);
    } else if ((flags & 2) && fn_8004935C() != 1) {
        if (SetPartSelected(1) != 0) PartSelectFeedback(0xCB8DAE3Eu);
    } else if (flags == 0 && fn_8004935C() != 4 && SetPartSelected(4) != 0) {
        PartSelectFeedback(0xD00B7E35u);
    }
}

// 0x800375F0
// Refresh of the HUD element set: when the refresh flag at +0x9A4 is set, re-finds the UI
// elements in the current screen set, updates the stored peak values and the "n/m" text,
// sets the scale/position of the marker element and clears the flag.
void fn_800375F0(u8* a) {
    u8* obj;
    u8* el;
    char buf[16];

    if (*(s32*)(a + 0x9A4) != 0) {
        fn_80038248(a);
        obj = *(u8**)(a + 0x980);
        (*(VFnI*)(*(u8**)(obj + 0x138) + 0x2C))(obj, 2);

        el = (u8*)ScreenSetsCurrent(gScreenSets);
        lbl_805FAFA4 = 0x8F972075u;
        if (el != NULL) {
            u8* set = el;
            *(void**)(a + 0x154) = ScreenSetFindElement(set, 0x0C0A089Au);
            *(void**)(a + 0x158) = ScreenSetFindElement(set, 0x8F972075u);
            *(void**)(a + 0x15C) = UIAnim_FindByName(set, 0xAF06E673u);
            *(void**)(a + 0x160) = UIAnim_FindByName(set, 0xDFA821B1u);
            *(void**)(a + 0x150) = ScreenSetFindElement(set, 0x728D86D2u);
            *(void**)(a + 0xA44) = ScreenSetFindElement(set, 0x79E9F640u);
            *(void**)(a + 0xA48) = ScreenSetFindElement(set, 0xB66AE04Eu);
            *(void**)(a + 0xA4C) = UIAnim_FindByName(set, 0xBC12E349u);
            *(void**)(a + 0xA50) = UIAnim_FindByName(set, 0x9437A55Au);

            el = (u8*)ScreenSetFindElement(set, 0x5A8E07B5u);
            *(void**)(a + 0xA64) = el;
            if (el != NULL) {
                if (*(f32*)(a + 0xA6C) == lbl_805FCEBC && *(f32*)(a + 0xA70) == lbl_805FCEBC &&
                    *(f32*)(a + 0xA74) == lbl_805FCEBC) {
                    *(f32*)(a + 0xA6C) = *(f32*)(el + 0x68);
                    *(f32*)(a + 0xA70) = *(f32*)(el + 0x6C);
                    *(f32*)(a + 0xA74) = *(f32*)(el + 0x70);
                    *(f32*)(a + 0xA68) = *(f32*)(el + 0x74);
                }
                obj = *(u8**)(a + 0xA64);
                *(f32*)(obj + 0x74) = lbl_805FCEBC;
                *(f32*)(obj + 0x68) = lbl_805FCEB8;
                obj = *(u8**)(a + 0xA64);
                (*(VFnIF*)(*(u8**)(obj + 0x138) + 0x2C))(obj, 0x38, lbl_805FCEBC);
            }
        }

        fn_8019BBA4(*(u8**)(a + 0x980), *(u8**)(a + 0x160), 0xA);

        el = *(u8**)(a + 0x150);
        if (el != NULL) {
            SysSprintf(buf, lbl_805F8498, *(s32*)(a + 0x16C), *(s32*)(a + 0x168));
            UISetText(el, buf, 1);
            (*(VFnI*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
        }

        fn_800223F0(a, lbl_803475F8[*(s32*)(a + 0x164)]);
        *(s32*)(a + 0x9A4) = 0;
    }
}

// 0x8002CBFC
// Screen-driven setup of the object's UI element sets: looks up the element for each of the
// five part slots (with its float offset and byte index), caches the element pointers and
// their positions, forces the X of each cached element, then copies the slot positions
// into the sub-objects and updates them.
void fn_8002CBFC(u8* obj) {
    u8* set;
    u8* el;
    u8* e5;
    u8* e6;
    u8* q;
    u8* sub;
    char buf[0x100];   // asm: 0x100 bytes at sp+0x18; the names are up to 34 characters ("fr_paper_doll_sparks_particle_%d")
    s32 i;

    fn_80021ECC();
    fn_800223F0(obj, *(f32*)(obj + 0xBC));
    set = (u8*)ScreenSetsCurrent(gScreenSets);

    e6 = (u8*)ScreenSetFindElement(set, 0xF92AEDD9u);
    e5 = (u8*)ScreenSetFindElement(set, 0xF190AAEFu);
    fn_8002C63C(obj + 0xF8, lbl_805F83E0[0], e5, (s32)(intptr_t)e6, *(f32*)(lbl_80357748 + 0x0));

    e6 = (u8*)ScreenSetFindElement(set, 0x24106BEEu);
    e5 = (u8*)ScreenSetFindElement(set, 0xD56BDF97u);
    fn_8002C63C(obj + 0x198, lbl_805F83E0[4], e5, (s32)(intptr_t)e6, *(f32*)(lbl_80357748 + 0x10));

    e6 = (u8*)ScreenSetFindElement(set, 0xCF9F95F2u);
    e5 = (u8*)ScreenSetFindElement(set, 0x5B354D6Cu);
    fn_8002C63C(obj + 0x148, lbl_805F83E0[2], e5, (s32)(intptr_t)e6, *(f32*)(lbl_80357748 + 0x8));

    e6 = (u8*)ScreenSetFindElement(set, 0xCA398B22u);
    e5 = (u8*)ScreenSetFindElement(set, 0x6BD28D6Cu);
    fn_8002C63C(obj + 0x170, lbl_805F83E0[3], e5, (s32)(intptr_t)e6, *(f32*)(lbl_80357748 + 0xC));

    e6 = (u8*)ScreenSetFindElement(set, 0xD9C5BBCCu);
    e5 = (u8*)ScreenSetFindElement(set, 0xAB7B1FF9u);
    fn_8002C63C(obj + 0x120, lbl_805F83E0[1], e5, (s32)(intptr_t)e6, *(f32*)(lbl_80357748 + 0x4));

    *(void**)(obj + 0xF4) = ScreenSetFindElement(set, 0xA33E6DBFu);

    for (i = 0; i < 5; i++) {
        SysSprintf(buf, (const char*)(lbl_80357748 + 0x14), i);
        el = (u8*)ScreenSetFindElement(set, StringHash(buf));
        *(void**)(obj + 0x1CC + i * 4) = el;
        *(f32*)(obj + 0x1E0 + i * 12) = *(f32*)(el + 0x68);
        *(f32*)(obj + 0x1E4 + i * 12) = *(f32*)(el + 0x6C);
        *(f32*)(obj + 0x1E8 + i * 12) = *(f32*)(el + 0x70);
        *(f32*)(el + 0x68) = lbl_805FCBB4;
    }

    for (i = 0; i < 3; i++) {
        SysSprintf(buf, (const char*)(lbl_80357748 + 0x38), i);
        el = (u8*)ScreenSetFindElement(set, StringHash(buf));
        *(void**)(obj + 0x1C0 + i * 4) = el;
    }

    *(void**)(obj + 0x3D4) = ScreenSetFindElement(set, 0x82BC0ED5u);

    q = obj;
    for (i = 0; i < 5; i++) {
        sub = *(u8**)(q + 0x114);
        *(f32*)(sub + 0x68) = *(f32*)(q + 0xF8);
        *(f32*)(sub + 0x6C) = *(f32*)(q + 0xFC);
        *(f32*)(sub + 0x70) = *(f32*)(q + 0x100);
        sub = *(u8**)(q + 0x114);
        (*(VFnIFF*)(*(u8**)(sub + 0x138) + 0x2C))(sub, 0x18, *(f32*)(q + 0xFC), *(f32*)(q + 0xF8));
        q += 0x28;
    }
}

}  // extern "C"
