// Battle / UI per-frame functions (0x8002xxxx - 0x8005xxxx).
#include "types.h"
#include "engine/math.h"
#include "game/part.h"
extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*ElemFn)(u8*, s32);          // UI element virtual (slot 0x2C / 0x30 of the table at +0x138)
typedef void (*ObjFn)(u8*, u8*);           // object virtual taking one pointer
typedef void (*ObjFnI)(u8*, s32);          // object virtual taking one int
typedef void (*PropSetFn)(u8*, s32, u32);  // dispatch table entry: set property (object, id, float bits)
typedef u8* (*ObjGetFn)(u8*);              // object virtual returning a pointer

extern "C" {
u8*   CharacterGetGround(u8* c);                              // 0x8005A520
void  CharacterSetMoved(u8* c);                               // 0x800533C8
void  CharacterSetRotated(u8* c);                             // 0x800533EC
void* ChoiceDestroy(void* c, s32 del);                        // 0x8020F394
u8*   FighterFromStats(u8* stats);                            // 0x80213F8C
s32   FighterStatsGet(u8* stats, s32 i);                      // 0x80213FB8
u8**  FighterStatsGetPart(u8* stats, s32 slot);               // 0x80213FE0
BOOL  PadButtonHeld(s32 button);                              // 0x801474EC
s32   PartGetBattleEnergy(const PartInstance* torso, u8** limbs);  // 0x8021ED90
void* PhysicsBodyGet(void* handle);                           // 0x801B1840
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e); // 0x80209E70
void  RayHitInit(u8* hit);                                    // 0x80055CD8
BOOL  RayQueryCast(u8* q, void* world, u32 filter, u8* hit, u8* hit2);   // 0x801B203C
const char* ReactionName(s32 id);                             // reaction_names.cpp
void  ScalarSet(f32* out, f32 v);                             // 0x80053250
void* ScreenSetFindElement(void* set, u32 hash);              // 0x80198F18
void* ScreenSetsCurrent(void* sets);                          // 0x8018D1E4
u32   StringHash(const char* s);                              // 0x801EB704
int   SysSprintf(char* buf, const char* fmt, ...);            // 0x80142EF8
char* strcpy(char* d, const char* s);
void  Vec3AddTo(Vec3* v, const Vec3* d);                      // 0x800544FC
void  Vec3MulScalar(Vec3* out, const Vec3* v, const f32* s);  // 0x80053224
void  Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);       // 0x80053370
void  fn_8003CB1C(u8* p);                                     // 0x8003CB1C
u8*   fn_8003CF14(u8* p);                                     // 0x8003CF14
void  fn_80041810(u8* dst, u8* src);                          // 0x80041810 (copy a choice)
u8*   fn_800526DC(u8* c);                                     // 0x800526DC
f32   fn_8005334C(u8* a3);                                    // 0x8005334C
u8*   fn_80053410(u8* c);                                     // 0x80053410
void  fn_8005379C(u8* obj, s32 slot, u8* other);              // 0x8005379C
u32   fn_80053ACC(u8* c, s32 mask);                           // 0x80053ACC
void  fn_80053AE8(u8* a3, s32 a4);                            // 0x80053AE8
u32   fn_80054A30(u8* c);                                     // 0x80054A30
void  fn_8005678C(u8* a3);                                    // 0x8005678C
u8*   fn_800567A4(u8* p, s32 del);                            // 0x800567A4
f32   fn_800567FC(u8* a3, u8* a4);                            // 0x800567FC
f32   fn_80056818(u8* a3, u8* a4);                            // 0x80056818
BOOL  fn_80056834(u8* obj);                                   // 0x80056834
u8    fn_8006C65C(u8* obj);                                   // 0x8006C65C
void  fn_80121508(char* text, s32 x, s32 y, s32 a, s32 b, s32 c, f32 d, f32 e);   // 0x80121508
void  fn_801458B0(f32* quat, f32* a, f32* b);                 // 0x801458B0
void  fn_80146180(void* quat, const Vec3* axis, f32 angle);   // 0x80146180
s32   fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2);              // 0x8019BBA4
s32   fn_801B19C4(u8* a, u32 hashA, u8* b, u32 hashB, u8* outA, u8* outB, s32 c, u8* d, u8 e);   // 0x801B19C4
s32   fn_801B1DD4(u8* listA, u32 idA, u8* listB, u32 idB, u8* outA, u8* outB, f32 limit);   // 0x801B1DD4
s32   fn_801B52AC(u8* p);                                     // 0x801B52AC
s32   fn_801B7A4C(u8* p, u8* obj);                            // 0x801B7A4C
void* fn_801D40E0(s32 n);                                     // 0x801D40E0
s32   fn_801DA780(u8* a3);                                    // 0x801DA780
u8*   fn_801DA788(u8* a3, s32 i);                             // 0x801DA788
void* fn_80209814(void* v);                                   // 0x80209814
void  fn_8020A398(s32 handle);                                // 0x8020A398

extern u8*  gBattle;                // 0x805FBBB8
extern u8   gWorldState[];          // 0x803E6D00 (first word: world block pointer)
extern u8   lbl_80348F38[];         // 0x80348F38 (dispatch table: +0x24 set property, indexed by type byte * 4)
extern u8   lbl_803C2A20[];         // 0x803C2A20
extern u8   lbl_803C4298[];         // 0x803C4298 (level info block)
extern f32  lbl_803C4D48[];         // 0x803C4D48 (wheel center, 3 floats)
extern s32  lbl_803C4E50[];         // 0x803C4E50 (debug angle table, degrees)
extern f32  lbl_80357858[];         // 0x80357858 (wheel marker angle offsets)
extern const char lbl_80354800[];   // 0x80354800 "Not found"
extern const char lbl_8035480C[];   // 0x8035480C "NOT FOUND"
extern const char lbl_805F8200[];   // 0x805F8200 (empty string)
extern u8   lbl_805FAFDC;           // 0x805FAFDC
extern u8   lbl_805FB044;           // 0x805FB044
extern s32  lbl_805FB048;           // 0x805FB048
extern s32  lbl_805FB04C;           // 0x805FB04C
extern u8   lbl_805FB050;           // 0x805FB050
extern u8   lbl_805FB051;           // 0x805FB051
extern f32  lbl_805FB05C;           // 0x805FB05C
extern f32  lbl_805FB060;           // 0x805FB060
extern f32  lbl_805FB064;           // 0x805FB064
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
extern f32  lbl_805FC958;           // 0x805FC958
extern f32  lbl_805FCBB0;           // 0x805FCBB0
extern f32  lbl_805FCBF8;           // 0x805FCBF8
extern f32  lbl_805FCBFC;           // 0x805FCBFC
extern f64  lbl_805FCC00;           // 0x805FCC00
extern f32  lbl_805FCC08;           // 0x805FCC08
extern f32  lbl_805FCC0C;           // 0x805FCC0C
extern f32  lbl_805FCFAC;           // 0x805FCFAC
extern f32  lbl_805FCFD0;           // 0x805FCFD0
extern f32  lbl_805FCFE4;           // 0x805FCFE4
extern f32  lbl_805FCFEC;           // 0x805FCFEC
extern f32  lbl_805FCFF4;           // 0x805FCFF4
extern f32  lbl_805FD018;           // 0x805FD018
extern f32  lbl_805FD098;           // 0x805FD098
extern f32  lbl_805FD0B4;           // 0x805FD0B4
extern f32  lbl_805FD0B8;           // 0x805FD0B8
extern f32  lbl_805FD118;           // 0x805FD118
extern f32  lbl_805FD128;           // 0x805FD128
extern f32  lbl_805FD138;           // 0x805FD138
extern f32  lbl_805FD13C;           // 0x805FD13C
extern f32  lbl_805FD358;           // 0x805FD358
}

extern "C++" void* gScreenSets;          // 0x805FB654
struct StringTable;
extern "C++" StringTable* gStringTable;   // 0x805FB910
extern "C++" StringTable* gStringTable2;  // 0x805FB90C

extern "C" {

// Creates the voice SFX bank object on first use (shared by the sound calls below).
static inline void EnsureVoiceBank(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// 0x80024958
// Round-slot highlight: depending on the mode at +0xE8 (4/3/2/other) shows one of the slot
// elements (+0x98..+0xA8) via virtual slot 0x2C and hides the rest (slot 0x30), stores the
// selection index at +0xAD8, nudges the header element's value at +0x6C by a constant, then
// refreshes it.
void fn_80024958(u8* arg0) {
    s32 mode = *(s32*)(arg0 + 0xE8);
    u8* el;
    switch (mode) {
    case 4:
        el = *(u8**)(arg0 + 0xA0);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
        el = *(u8**)(arg0 + 0xA4);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        el = *(u8**)(arg0 + 0xA8);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        *(s32*)(arg0 + 0xAD8) = 0;
        break;
    case 3:
        el = *(u8**)(arg0 + 0xA0);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        el = *(u8**)(arg0 + 0xA4);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
        el = *(u8**)(arg0 + 0xA8);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        el = *(u8**)(arg0 + 0x98);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        *(s32*)(arg0 + 0xAD8) = 1;
        el = *(u8**)(arg0 + 0x90);
        *(f32*)(el + 0x6C) = lbl_805FC958 + *(f32*)(el + 0x6C);
        break;
    case 2:
        el = *(u8**)(arg0 + 0xA0);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        el = *(u8**)(arg0 + 0xA4);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        el = *(u8**)(arg0 + 0xA8);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
        el = *(u8**)(arg0 + 0x9C);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        *(s32*)(arg0 + 0xAD8) = 2;
        el = *(u8**)(arg0 + 0x90);
        *(f32*)(el + 0x6C) = lbl_805FC958 + *(f32*)(el + 0x6C);
        break;
    case 1:
    case 0:
        el = *(u8**)(arg0 + 0xA0);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        el = *(u8**)(arg0 + 0xA4);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        el = *(u8**)(arg0 + 0xA8);
        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        *(s32*)(arg0 + 0xAD8) = 3;
        el = *(u8**)(arg0 + 0x90);
        *(f32*)(el + 0x6C) = lbl_805FC958 + *(f32*)(el + 0x6C);
        break;
    default:
        break;
    }
    el = *(u8**)(arg0 + 0x90);
    (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
}

// 0x8002D83C
// Spins the wheel element (+0xF4) by its speed (+0xAC) * scale (+0x3D0) * dt, wrapping at the
// full-turn constant, then places the three marker elements (+0x1C0..) on a circle around the
// "fr_wheel_center_marker" element (position cached in lbl_803C4D48 on first use).
void fn_8002D83C(u8* arg0, f32 dt) {
    u8* wheel = *(u8**)(arg0 + 0xF4);
    if (wheel == NULL) return;

    f32 ang = *(f32*)(wheel + 0x98) + *(f32*)(arg0 + 0x3D0) * (lbl_805FCBF8 * (*(f32*)(arg0 + 0xAC) * dt));
    if (!(ang < lbl_805FCBFC)) {
        ang -= lbl_805FCBFC;
    }
    f32 zero = lbl_805FCBB0;
    *(f32*)(wheel + 0x90) = zero;
    *(f32*)(wheel + 0x94) = zero;
    *(f32*)(wheel + 0x98) = ang;
    wheel = *(u8**)(arg0 + 0xF4);
    (*(ElemFn*)(*(u8**)(wheel + 0x138) + 0x2C))(wheel, 0x18);

    if (lbl_805FAFDC == 0) {
        void* sets = gScreenSets;
        u32 hash = StringHash("fr_wheel_center_marker");
        u8* el = (u8*)ScreenSetFindElement(ScreenSetsCurrent(sets), hash);
        lbl_803C4D48[0] = *(f32*)(el + 0x68);
        lbl_803C4D48[1] = *(f32*)(el + 0x6C);
        lbl_803C4D48[2] = *(f32*)(el + 0x70);
        lbl_805FAFDC = 1;
    }

    f32 radius = (f32)(lbl_805FCC00 * (f64)*(f32*)(*(u8**)(arg0 + 0xF4) + 0x74));
    for (s32 i = 0; i < 3; i++) {
        u8* marker = *(u8**)(arg0 + 0x1C0 + i * 4);
        f32 z = *(f32*)(marker + 0x70);
        f32 a = lbl_805FCC08 * (lbl_805FCC0C * (ang + lbl_80357858[i]));
        f32 c = cosf(a);
        f32 s = sinf(a);
        marker = *(u8**)(arg0 + 0x1C0 + i * 4);
        f32 y = radius * c + zero * s;
        f32 x = zero * c + radius * -s;
        *(f32*)(marker + 0x68) = lbl_803C4D48[0] + x;
        *(f32*)(marker + 0x6C) = lbl_803C4D48[1] + y;
        *(f32*)(marker + 0x70) = lbl_803C4D48[2] + z;
    }
}

// 0x8003B2DC
// Per-frame update of the six pickup/fly-in effect slots. Slot i has an element at +0x244+4i
// (position at +0x68), an "outgoing" flag byte at +0x29A+i, a timer at +0x2F0+12i (velocity
// at +0x2E8/+0x2EC), a target time at +0x360+4i and a result index at +0x330+4i. A flagged slot
// counts down: at zero it retires its result entry (fn_8003CB1C, sound, counters) and hides.
// An unflagged slot counts up, moves along its velocity and lerps its scale (+0x380..+0x384);
// when the timer reaches the target it completes (counter changes, sound, rumble).
void fn_8003B2DC(u8* arg0, f32 dt) {
    f32 zero = lbl_805FCFAC;
    f32 hidden = lbl_805FCFF4;
    f32 three = lbl_805FD018;
    f32 one = lbl_805FCFEC;
    f32 step = lbl_805FCFE4;

    for (s32 i = 0; i < 6; i++) {
        u8* slotEl = *(u8**)(arg0 + 0x244 + i * 4);
        if (!(*(f32*)(slotEl + 0x68) > hidden)) continue;

        u8* fx = arg0 + i * 0xC;
        f32 t = *(f32*)(fx + 0x2F0);
        f32 tn;
        if (*(u8*)(arg0 + 0x29A + i) != 0) {
            tn = t - dt;
            if (tn <= zero) {
                s32 res = *(s32*)(arg0 + 0x330 + i * 4);
                if (res != -1) {
                    fn_8003CB1C(arg0 + res * 0x34 + 0x90);
                    *(s32*)(arg0 + 0x950) = 2;
                    f32 py = ((f32*)fn_8003CF14(arg0 + *(s32*)(arg0 + 0x330 + i * 4) * 0x34 + 0x90))[1];
                    s32 sel = *(s32*)(arg0 + 0x950);
                    f32 px = *(f32*)fn_8003CF14(arg0 + *(s32*)(arg0 + 0x330 + i * 4) * 0x34 + 0x90);
                    u8* el;
                    switch (sel) {
                    case 0:
                        el = *(u8**)(arg0 + 0x944);
                        *(f32*)(el + 0x68) = px;
                        *(f32*)(el + 0x6C) = py;
                        *(f32*)(el + 0x70) = zero;
                        el = *(u8**)(arg0 + 0x944);
                        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x1A);
                        break;
                    case 1:
                        el = *(u8**)(arg0 + 0x948);
                        *(f32*)(el + 0x68) = px;
                        *(f32*)(el + 0x6C) = py;
                        *(f32*)(el + 0x70) = zero;
                        el = *(u8**)(arg0 + 0x948);
                        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x1A);
                        break;
                    case 2:
                        el = *(u8**)(arg0 + 0x94C);
                        *(f32*)(el + 0x68) = px;
                        *(f32*)(el + 0x6C) = py;
                        *(f32*)(el + 0x70) = zero;
                        el = *(u8**)(arg0 + 0x94C);
                        (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x1A);
                        break;
                    default:
                        break;
                    }
                    s32 oldCount = *(s32*)(arg0 + 0x3B8);
                    *(s32*)(arg0 + 0x3B8) = oldCount - 1;
                    *(s32*)(arg0 + 0x330 + i * 4) = -1;
                    // virtual slot 0x60 of the object itself; the old count is left in r4 (passed on)
                    (*(ObjFnI*)(*(u8**)(arg0 + 4) + 0x60))(arg0, oldCount);
                    EnsureVoiceBank();
                    PlaySound(0x9825C47Eu, 0, 0, 0, 0, 0);
                    s32 cnt = *(s32*)(arg0 + 0x3B8);
                    if (cnt == *(s32*)(arg0 + 0x3BC)) {
                        fn_8020A398(*(s32*)(arg0 + 0x3C4));
                        EnsureVoiceBank();
                        *(s32*)(arg0 + 0x3C4) = PlaySound(0x9A5F41CEu, 0, 0, 0, 1, 0);
                    } else if (cnt == *(s32*)(arg0 + 0x3C0)) {
                        fn_8020A398(*(s32*)(arg0 + 0x3C4));
                        EnsureVoiceBank();
                        *(s32*)(arg0 + 0x3C4) = PlaySound(0xEA2B9788u, 0, 0, 0, 1, 0);
                    }
                }
                *(f32*)(*(u8**)(arg0 + 0x244 + i * 4) + 0x68) = hidden;
                *(u8*)(arg0 + 0x29A + i) = 0;
                *(f32*)(fx + 0x2F0) = zero;
                u8* el = *(u8**)(arg0 + 0x244 + i * 4);
                (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 8);
                *(s32*)(arg0 + 0x3A4) = *(s32*)(arg0 + 0x3A4) - 1;
                continue;
            }
        } else {
            tn = t + dt;
        }

        *(f32*)(fx + 0x2F0) = tn;
        if (tn >= *(f32*)(arg0 + 0x360 + i * 4)) {
            (*(ObjGetFn*)(*(u8**)(arg0 + 4) + 0x68))(arg0);
            *(s32*)(arg0 + 0x998) = 1;
            *(f32*)(arg0 + 0x99C) = three;
            fn_8019BBA4(*(u8**)(arg0 + 0x930), *(u8**)(arg0 + 0x92C), 0xA);
            *(f32*)(*(u8**)(arg0 + 0x244 + i * 4) + 0x68) = hidden;
            *(f32*)(fx + 0x2E8) = zero;
            *(f32*)(fx + 0x2EC) = zero;
            *(f32*)(fx + 0x2F0) = zero;
            u8* el = *(u8**)(arg0 + 0x244 + i * 4);
            (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 8);
            s32 n = *(s32*)(arg0 + 0x3B4);
            if (n < 8) {
                el = *(u8**)(arg0 + 0x25C + n * 4);
                (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
                *(s32*)(arg0 + 0x3B4) = *(s32*)(arg0 + 0x3B4) + 1;
            }
            *(s32*)(arg0 + 0x3A4) = *(s32*)(arg0 + 0x3A4) - 1;
            EnsureVoiceBank();
            PlaySound(0xBE4A33CCu, 0, 0, 0, 0, 0);
            TimedRumble(*(s32*)(lbl_803C4298 + 0x38), lbl_805FCFD0, 4);
        } else {
            u8* el = *(u8**)(arg0 + 0x244 + i * 4);
            f32 px = *(f32*)(el + 0x68) + dt * *(f32*)(fx + 0x2E8);
            f32 py = *(f32*)(el + 0x6C) + dt * *(f32*)(fx + 0x2EC);
            f32 pz = *(f32*)(el + 0x70);
            *(f32*)(el + 0x68) = px;
            *(f32*)(el + 0x6C) = py;
            *(f32*)(el + 0x70) = pz;
            f32 frac = tn / *(f32*)(arg0 + 0x360 + i * 4);
            f32 s0 = *(f32*)(arg0 + 0x380);
            f32 sc = s0 + (*(f32*)(arg0 + 0x384) - s0) * frac;
            el = *(u8**)(arg0 + 0x244 + i * 4);
            *(f32*)(el + 0x74) = sc;
            *(f32*)(el + 0x78) = sc;
            *(f32*)(el + 0x7C) = one;
            el = *(u8**)(arg0 + 0x244 + i * 4);
            *(f32*)(el + 0x98) = *(f32*)(el + 0x98) + step;
            el = *(u8**)(arg0 + 0x244 + i * 4);
            (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 8);
        }
    }
}

// 0x8003E560
// Debug camera/angle tweaking for fighter slot `idx` (pad buttons 0x1086/0x1087/0x1088 cycle and
// nudge lbl_803C4E50 angles, 0x1085 reads the axis), then for each of the five entries of the
// fighter table feeds the resulting Y axis and the world's two vectors into the parts' hit
// records (property ids 0x135-0x137 and 0x13A-0x13C) when the world flag word has both low bits.
// Always returns 0.
s32 fn_8003E560(s32 idx) {
    if (lbl_805FB044 != 0) return 0;

    u8** table = *(u8***)(gBattle + idx * 4 + 0xA00);
    if (lbl_805FB050 != 0) {
        if (PadButtonHeld(0x1086)) {
            if (!(lbl_805FB048 & 0x1086)) {
                lbl_805FB048 |= 0x1086;
                lbl_805FB04C = (lbl_805FB04C + 1) % 4;
            }
        } else {
            lbl_805FB048 &= ~0x1086;
        }
        if (PadButtonHeld(0x1087)) {
            s32 k = lbl_805FB04C;
            lbl_803C4E50[k] = (lbl_803C4E50[k] + 1) % 360;
        }
        if (PadButtonHeld(0x1088)) {
            s32 k = lbl_805FB04C;
            lbl_803C4E50[k] = (lbl_803C4E50[k] - 1) % 360;
        }
    }

    u8* owner = table[7];   // +0x1C
    if (owner == NULL) return 0;
    owner -= 0xC48;
    f32 angX = *(f32*)(owner + 0xE0);
    f32 angY = *(f32*)(owner + 0xE4);

    f32 zero = lbl_805FD098;
    f32 one = lbl_805FD0B4;
    Quat q;
    Vec3 up = { zero, one, zero };
    Vec3 fwd = { zero, zero, one };
    fn_801458B0((f32*)&q, (f32*)&fwd, (f32*)&up);
    Vec3 axisX = { one, zero, zero };
    fn_80146180(&q, &axisX, lbl_805FD0B8 * ((f32)lbl_803C4E50[idx + 2] + angX));
    Vec3 axisY = { zero, one, zero };
    fn_80146180(&q, &axisY, lbl_805FD0B8 * ((f32)lbl_803C4E50[idx] + angY));
    if (lbl_805FB050 != 0 && PadButtonHeld(0x1085)) {
        QuatGetAxisY(&q);
    }

    u8* world = *(u8**)gWorldState;
    if (*(s32*)(world + 0x1A7C) < 1) return 0;

    f32 v1[3], v0[3];
    v1[0] = *(f32*)(world + 0x204);
    v1[1] = *(f32*)(world + 0x208);
    v1[2] = *(f32*)(world + 0x20C);
    v0[0] = *(f32*)(world + 0x1F8);
    v0[1] = *(f32*)(world + 0x1FC);
    v0[2] = *(f32*)(world + 0x200);

    for (s32 e = 0; e < 5; e++) {
        u8* slot = table[e];
        u8* fighter = slot != NULL ? *(u8**)(slot + 0x20) : NULL;
        if (fighter != NULL) fighter -= 0x264;
        if (fighter == NULL || fn_8006C65C(fighter) != 0) continue;

        u8* list = (*(ObjGetFn*)(*(u8**)fighter + 0xC))(fighter);
        for (s32 a = 0; a < fn_801DA780(list); a++) {
            Vec3 dir = QuatGetAxisY(&q);
            u8* rec = fn_801DA788(list, a);
            *(f32*)(rec + 0xA4) = dir.x;
            *(f32*)(rec + 0xA8) = dir.y;
            *(f32*)(rec + 0xAC) = dir.z;
            if (lbl_805FB051 != 0 || (*(u32*)(world + 0xA20) & 3) != 3) continue;

            s32 off = 0;
            for (s32 p = 0; p < *(s32*)(*(u8**)(fn_801DA788(list, a) + 0x10) + 0x6C); p++) {
                u8* part = *(u8**)(*(u8**)(fn_801DA788(list, a) + 0x10) + off + 0x70);
                if (part != NULL && *(u8*)(part + 0xC) == 1) {
                    f32 vals[6] = { v0[0], v0[1], v0[2], v1[0], v1[1], v1[2] };
                    static const s32 ids[6] = { 0x135, 0x136, 0x137, 0x13A, 0x13B, 0x13C };
                    for (s32 k = 0; k < 6; k++) {
                        // property value goes in r5 as the raw float bits
                        u32 bits = *(u32*)&vals[k];
                        (*(PropSetFn*)(lbl_80348F38 + *(u8*)(part + 0xC) * 4 + 0x24))(*(u8**)(part + 8), ids[k], bits);
                    }
                }
                off += 8;
            }
        }
    }
    return 0;
}

// 0x800412DC
// Battle debug overlay: for both fighters prints the chosen action, energy / stat numbers and
// for each of the five part slots the part name with tier, defIndex / defect and a damage value;
// then three timers (round, total combat, total animation, last combat).
void fn_800412DC(u8* arg0, f32 dt) {
    char buf[0x100];
    u8 saved = lbl_803C2A20[0x820];
    lbl_803C2A20[0x820] = 1;

    SysSprintf(buf, "ROUND: %d", *(s32*)(gBattle + 0xA10));
    fn_80121508(buf, 0x140, 0x32, (s32)0xFF00FFFF, 1, 0, lbl_805FD138, lbl_805FD118);

    for (s32 f = 0; f < 2; f++) {
        s32 yRow = f * 0x1E;
        s32 xCol = f * 0x10E;
        s32 dmgOff = f * 0x38;
        s32 xBlk = f * 0x12C;

        u8* stats = *(u8**)(gBattle + f * 4 + 0xA00) + 0x20;
        s32 choice[3];
        fn_80041810((u8*)choice, stats);
        s32 id = choice[0];
        ChoiceDestroy(choice, -1);
        if (id == -1) continue;

        // The action table entry (0x14 bytes) is also copied twice onto the stack here
        // (dead struct temporaries); only its +4 field (action type) is used.
        u8* act = *(u8**)(gGameDb->data + 0x430) + id * 0x14;
        strcpy(buf, ReactionName(*(s32*)(act + 4)));
        fn_80121508(buf, 0x12C, yRow + 0x64, -1, 1, 0, lbl_805FD13C, lbl_805FD118);

        u8* torso = *(u8**)*(u8**)(FighterFromStats(stats) + 0x10);
        s32 energy = PartGetBattleEnergy((const PartInstance*)torso, (u8**)FighterFromStats(stats));
        SysSprintf(buf, "%d / %d", FighterStatsGet(stats, 3), energy);
        fn_80121508(buf, xCol + 0xB9, 0x3C, -1, 1, 1, lbl_805FD128, lbl_805FD118);
        SysSprintf(buf, "%d", FighterStatsGet(stats, 1));
        fn_80121508(buf, xCol + 0x87, 0x3C, -1, 1, 1, lbl_805FD128, lbl_805FD118);

        for (s32 j = 0; j < 5; j++) {
            s32 yPart = j * 0x14;
            u8** slotp = *(u8***)(FighterFromStats(stats) + j * 4);
            if (slotp != NULL) {
                u8* inst = *slotp;
                u8** gp = FighterStatsGetPart(stats, j);
                PartDef def;
                PartDbGetDef(&def, gGameDb, *(s32*)inst);
                // (the 0x50-byte definition is copied once more on the stack; only +4 is read)
                s32 tid = TextIdFromHash(def.tierName[0]);
                const char* name;
                if ((u32)(tid + 0x10000) == 0xFFFF) {
                    name = lbl_80354800;
                } else if ((tid & 0x80000000) && gStringTable2 != NULL) {
                    u8* t2 = (u8*)gStringTable2;
                    u8* arr = *(u8**)t2;
                    if (arr != NULL && (u32)(tid & 0x7FFFFFFF) < *(u32*)(t2 + 0xC)) {
                        name = *(const char**)(arr + ((u32)tid << 2));
                    } else if ((u32)((tid & 0x7FFFFFFF) + 0x10000) == 0xFFFF) {
                        name = lbl_8035480C;
                    } else {
                        name = lbl_805F8200;
                    }
                } else {
                    u8* t1 = (u8*)gStringTable;
                    u8* arr = *(u8**)t1;
                    if (arr != NULL && (u32)tid < *(u32*)(t1 + 0xC)) {
                        name = *(const char**)(arr + ((u32)tid << 2));
                    } else if ((u32)(tid + 0x10000) == 0xFFFF) {
                        name = lbl_8035480C;
                    } else {
                        name = lbl_805F8200;
                    }
                }
                SysSprintf(buf, "%s (%d)", name, (s32)PartGetTier((const PartInstance*)inst));
                fn_80121508(buf, xBlk + 0x78, yPart + 0x118, -1, 1, 1, lbl_805FD13C, lbl_805FD118);
                u8* cur = *gp;
                SysSprintf(buf, "%d / %d", *(s32*)cur, *(s32*)(cur + 4));
                fn_80121508(buf, xBlk + 0xC8, yPart + 0x118, -1, 1, 1, lbl_805FD13C, lbl_805FD118);
            }

            // Damage dealt to this slot (negated), 0 when the hit record chain is missing.
            s32 dmg = 0;
            u8* hits = *(u8**)(gBattle + 0xA0C);
            if (hits != NULL) {
                u8* h = *(u8**)(hits + dmgOff + j * 4 + 4);
                if (h != NULL) {
                    u8** q = *(u8***)(h + 4);
                    if (q != NULL) {
                        dmg = -**(s32**)q;
                    }
                }
            }
            SysSprintf(buf, "%d", dmg);
            fn_80121508(buf, xBlk + 0xF0, yPart + 0x118, -1, 1, 1, lbl_805FD13C, lbl_805FD118);
        }
    }

    lbl_805FB05C = lbl_805FB05C + dt;
    SysSprintf(buf, "Total combat time: %.0f", (f64)lbl_805FB05C);
    fn_80121508(buf, 0x1A4, 0x64, (s32)0xFF00FFFF, 1, 0, lbl_805FD13C, lbl_805FD118);
    SysSprintf(buf, "Total animation time: %.0f", (f64)lbl_805FB060);
    fn_80121508(buf, 0x1A4, 0x78, (s32)0xFF00FFFF, 1, 0, lbl_805FD13C, lbl_805FD118);
    SysSprintf(buf, "Last combat time: %.0f", (f64)lbl_805FB064);
    fn_80121508(buf, 0x1A4, 0x8C, (s32)0xFF00FFFF, 1, 0, lbl_805FD13C, lbl_805FD118);

    lbl_803C2A20[0x820] = saved;
}

// 0x8005628C
// Collision response between two characters, dispatched on the type of the second (fn_801B52AC):
// type 4 (solid), 6 (swept) and 8 (trigger). Types 4 / 6 run the part-vs-part hit test
// (fn_801B19C4) and notify both objects (virtual 0x5C / 0x60), then call virtual 0xBC (arg0, arg1).
void fn_8005628C(u8* arg0, u8* arg1) {
    u8 hitA[0x6C], hitB[0x6C];
    RayHitInit(hitA);
    RayHitInit(hitB);
    s32 type = fn_801B52AC(arg1);

    if (type == 4) {
        if (fn_80053ACC(arg0, 0x40000000) != 0) return;
        u8 h0[0x6C], h1[0x6C];
        RayHitInit(h0);
        RayHitInit(h1);
        u8* extra = fn_80054A30(arg0) != 0 ? arg0 + 0x7A4 : NULL;
        s32 airborne = 0;
        if (fn_80054A30(arg0) != 0 && CharacterGetGround(arg0) == NULL) airborne = 1;
        if (fn_801B19C4(*(u8**)(arg0 + 0x8C), 0x7F20C9C6u, *(u8**)(arg1 + 0x8C), 0x7F20C9C6u, h0, h1, airborne, extra, 0) != 0) {
            (*(ObjFn*)(*(u8**)arg0 + 0x5C))(arg0, h0);
            (*(ObjFn*)(*(u8**)arg1 + 0x5C))(arg1, h1);
        } else {
            extra = fn_80054A30(arg0) != 0 ? arg0 + 0x7A4 : NULL;
            airborne = 0;
            if (fn_80054A30(arg0) != 0 && CharacterGetGround(arg0) == NULL) airborne = 1;
            if (fn_801B19C4(*(u8**)(arg0 + 0x8C), 0x7F20C9C6u, *(u8**)(arg1 + 0x8C), 0xF62113D5u, h0, h1, airborne, extra, 0) != 0) {
                fn_8005379C(arg0, 6, arg1);
                fn_8005379C(arg1, 6, arg0);
            }
        }
        (*(ObjFn*)(*(u8**)arg0 + 0xBC))(arg0, arg1);
        return;
    }

    if (type == 6) {
        if (fn_80053ACC(arg0, 0x02000000) != 0) return;
        s32 blocked = 0;
        if (arg0 == *(u8**)(*(u8**)gWorldState + 8)) {
            // Player character: sweep from the previous to the current position first.
            u8 query[0x6C], sweepA[0x6C], sweepB[0x6C];
            fn_8005678C(query);
            *(s32*)(query + 0xC) = 5;
            RayHitInit(sweepA);
            RayHitInit(sweepB);
            u8* body = (u8*)PhysicsBodyGet(*(void**)(arg0 + 0x8C));
            fn_800567FC(query, body + 0x20);
            Vec3 delta;
            Vec3Sub(&delta, (const Vec3*)(body + 0x14), (const Vec3*)(body + 0x20));
            fn_80056818(query, (u8*)&delta);
            if (RayQueryCast(query, *(void**)(arg1 + 0x8C), 0xF62113D5u, sweepA, sweepB) != 0) {
                if (fn_80053ACC(arg1, 0x02000000) == 0) {
                    f32 scale;
                    ScalarSet(&scale, lbl_805FD358);
                    Vec3 d, push;
                    Vec3Sub(&d, (const Vec3*)sweepA, (const Vec3*)(body + 0x14));
                    Vec3MulScalar(&push, &d, &scale);
                    Vec3AddTo((Vec3*)fn_800526DC(arg0), &push);
                    Vec3AddTo((Vec3*)(body + 0x14), &push);
                    CharacterSetMoved(arg0);
                    CharacterSetRotated(arg0);
                } else {
                    blocked = 1;
                    fn_8005379C(arg0, 6, arg1);
                    fn_8005379C(arg1, 6, arg0);
                }
            }
            fn_800567A4(query, -1);
        }
        u8 h0[0x6C], h1[0x6C];
        RayHitInit(h0);
        RayHitInit(h1);
        u8* extra = fn_80054A30(arg0) != 0 ? arg0 + 0x7A4 : NULL;
        s32 airborne = 0;
        if (fn_80054A30(arg0) != 0 && CharacterGetGround(arg0) == NULL) airborne = 1;
        if (fn_801B19C4(*(u8**)(arg0 + 0x8C), 0x7F20C9C6u, *(u8**)(arg1 + 0x8C), 0xF62113D5u, h0, h1, airborne, extra, 0) != 0) {
            (*(ObjFn*)(*(u8**)arg0 + 0x60))(arg0, h0);
            fn_80053AE8(arg0 + 0x838, 0x10);
            if (blocked == 0) {
                fn_8005379C(arg0, 6, arg1);
                fn_8005379C(arg1, 6, arg0);
            }
        }
        (*(ObjFn*)(*(u8**)arg0 + 0xBC))(arg0, arg1);
        return;
    }

    if (type == 8) {
        if (fn_80053ACC(arg1, 0x04000000) != 0) return;
        if (fn_80056834(arg1) == 0) return;
        u8* link = *(u8**)(arg1 + 0x204);
        if (link != NULL && fn_801B7A4C(link, arg0) == 0) return;
        u8 h0[0x6C], h1[0x6C];
        RayHitInit(h0);
        RayHitInit(h1);
        u32 filter = 0x7F20C9C6u;
        if (PhysicsBodyGet(*(void**)fn_80053410(arg0)) == NULL) filter = 0xF62113D5u;
        fn_8005334C(arg0);
        // f1 (limit) is not set before this call in the asm: leftover, passed as 0
        if (fn_801B1DD4(*(u8**)(arg0 + 0x8C), filter, *(u8**)(arg1 + 0x8C), 0x4C7ED036u, h0, h1, 0.0f) != 0) {
            (*(ObjFn*)(*(u8**)arg1 + 0xBC))(arg1, h1);
        }
    }
}

}  // extern "C"
