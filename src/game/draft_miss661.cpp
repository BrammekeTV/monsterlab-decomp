// Catch minigame: fly/bug lane setup (fn_80028340), tomato-throw minigame update and setup
// (fn_80034B98, fn_80035DB8, fn_8003A2B4), and the lane marker wobble (fn_800267FC).
#include "types.h"

typedef void (*ElemFn)(void*, s32);
typedef void (*ObjFn)(void*);

extern "C" {
// Script callbacks (declared here, not through script_callbacks.h, because Print takes a string
// pointer; see the note at its use).
void StopRumble(s32 chan);                                           // 0x801B2B94
void UIElement_SetVisibleFlag(u32 a0, u32 a1);                       // src/game/draft_miss1.cpp
void GameFlowSignal(s32 id, s32 arg1, s32 arg2);                     // 0x8000C7F4
void Print(const char* s);                                           // 0x801DB498

void CatchMinigame_UpdateBase(void* g, f32 dt);                      // 0x800224C4
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
s32  PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);         // 0x80209E70
u32  RandomU32(void);                                                // 0x80143460
void* ScreenSetFindElement(void* set, u32 hash);                     // 0x80198F18
void* ScreenSetsCurrent(void* sets);                                 // 0x8018D1E4
void StickRead(s32 channel, f32* x, f32* y);                         // 0x80147704
u32  StringHash(const char* s);                                      // 0x801EB704
int  SysSprintf(char* buf, const char* fmt, ...);                    // 0x80142EF8
s32  __cvt_fp2signed(s32 v);                                         // 0x80332ED0 (integer abs)
f32  cosf(f32 x);                                                    // 0x8033EC98 (MSL)
f32  sinf(f32 x);                                                    // 0x8033F0A4 (MSL)
void* memset(void* p, int c, unsigned long n);

void fn_80021ECC(void);                                              // 0x80021ECC
void fn_800223F0(u8* p, f32 v);                                      // 0x800223F0
void fn_800225C8(void* a0);                                          // 0x800225C8
void fn_800294BC(u8* lane, void* el, void* e1, void* e2, s32 v);     // 0x800294BC
void fn_800296C4(u8* lane, u8* p1, u8* p2, u8* box);                 // 0x800296C4
void fn_8002A178(u8* lane, void* v);                                 // 0x8002A178
u8*  fn_8002A7F8(u8* lane, void* el);                                // 0x8002A7F8
void fn_8002A8CC(void);                                              // 0x8002A8CC
u8*  fn_8002B58C(u8* obj, void* el, s32 v, u8* p1, u8* p2, u8* box); // 0x8002B58C
void fn_8002BD60(u8* obj);                                           // 0x8002BD60
void fn_800311E8(u8* box, f32* pt);                                  // 0x800311E8
void fn_800359F4(u8* a3, f32 f1, f32 f2, f32 f3);                    // 0x800359F4
void fn_80035164(u8* c, s32 team, f32 dt);                           // 0x80035164
void fn_80035A04(u8* c, s32 n);                                      // 0x80035A04
void fn_80035DB8(u8* c, f32 dt);                                     // 0x80035DB8
void fn_80036618(u8* arg0, f32 farg0);                               // 0x80036618
s32  fn_8003639C(u8* c);                                             // 0x8003639C
void fn_8003CE20(u8* arg0, s32 arg1, s32 arg2);                      // 0x8003CE20
void* fn_80198F1C(void* set, u32 hash);                              // 0x80198F1C
ScriptValue fn_801A1208(u8* el);                                        // 0x801A1208 (ScriptValue: image element -> texture)
void fn_801A8654(u8* obj, s32 a1);                                   // 0x801A8654
u8*  fn_801B3308(void);                                              // 0x801B3308
void fn_801B3310(u8* p, u8* q);                                      // 0x801B3310
void* fn_801D40E0(s32 n);                                            // 0x801D40E0
void* fn_80209814(void* v);                                          // 0x80209814
u32  fn_8020A9A8(s32 handle);                                        // 0x8020A9A8

extern void* gLab;                    // 0x805FBBC8
extern u8  lbl_803C4298[];            // 0x803C4298
extern u8  lbl_805FAFC4;              // 0x805FAFC4
extern f32 lbl_805FAFF8;              // 0x805FAFF8
extern f32 lbl_805FAFFC;              // 0x805FAFFC
extern s32 lbl_805FB000;              // 0x805FB000 (frame counter)
extern f32 lbl_805FB004;              // 0x805FB004
extern void* lbl_805FBBAC;            // 0x805FBBAC (voice SFX handle)
extern s32 lbl_805F8480;              // 0x805F8480
extern f32 lbl_805F8484;              // 0x805F8484
extern s32 lbl_805F8488;              // 0x805F8488

extern f32 lbl_805FC990;              // 0x805FC990 (0.0f)
extern f32 lbl_805FC9C4;              // 0x805FC9C4 (2.3283064e-10f, 1 / 2^32)
extern f32 lbl_805FC9C8;              // 0x805FC9C8 (-45.0f)
extern f32 lbl_805FC9CC;              // 0x805FC9CC (90.0f)
extern f32 lbl_805FC9D0;              // 0x805FC9D0 (65.0f)
extern f32 lbl_805FC9D4;              // 0x805FC9D4 (20.0f)
extern f32 lbl_805FC9D8;              // 0x805FC9D8 (0.08726646f)
extern f32 lbl_805FC9DC;              // 0x805FC9DC (-1.0f)
extern f32 lbl_805FC9E0;              // 0x805FC9E0 (0.017453292f)
extern f32 lbl_805FC9E4;              // 0x805FC9E4 (0.7f)
extern f32 lbl_805FC9E8;              // 0x805FC9E8 (640.0f)
extern f32 lbl_805FC9EC;              // 0x805FC9EC (480.0f)
extern f32 lbl_805FCA38;              // 0x805FCA38 (0.0f)
extern f32 lbl_805FCA44;              // 0x805FCA44 (0.25f)
extern f64 lbl_805FCA48;              // 0x805FCA48 (0.5)
extern f32 lbl_805FCDBC;              // 0x805FCDBC (0.0f)
extern f32 lbl_805FCDD0;              // 0x805FCDD0 (114.0f)
extern f32 lbl_805FCDE0;              // 0x805FCDE0 (0.5f)
extern f32 lbl_805FCE14;              // 0x805FCE14 (1.0f)
extern f32 lbl_805FCE20;              // 0x805FCE20 (-1.0f)
extern f32 lbl_805FCE58;              // 0x805FCE58 (0.1f)
extern f32 lbl_805FCE5C;              // 0x805FCE5C (0.033333335f)
extern f32 lbl_805FCE90;              // 0x805FCE90 (0.25f)
extern f32 lbl_805FCE9C;              // 0x805FCE9C (0.05f)
extern f32 lbl_805FCEA0;              // 0x805FCEA0 (-0.05f)
extern f32 lbl_805FCEA4;              // 0x805FCEA4 (526.0f)
extern f32 lbl_805FCEA8;              // 0x805FCEA8 (114.5f)
extern f32 lbl_805FCEAC;              // 0x805FCEAC (525.5f)
extern f32 lbl_805FCEB0;              // 0x805FCEB0 (0.3f)
extern f32 lbl_805FCF98;              // 0x805FCF98 (25.0f)
extern f32 lbl_805FCF9C;              // 0x805FCF9C (300.0f)
extern f32 lbl_805FCFA0;              // 0x805FCFA0 (35.0f)
extern f32 lbl_805FCFA4;              // 0x805FCFA4 (330.0f)
extern f32 lbl_805FCFAC;              // 0x805FCFAC (0.0f)
extern f32 lbl_805FCFC8;              // 0x805FCFC8 (1.55f)
extern f32 lbl_805FCFCC;              // 0x805FCFCC (0.6f)
extern f32 lbl_805FCFD0;              // 0x805FCFD0 (0.4f)
extern f32 lbl_805FCFD4;              // 0x805FCFD4 (700.0f)
extern f32 lbl_805FCFB8;              // 0x805FCFB8 (0.5f)
}

extern "C++" void* gEventMgr;         // 0x805FB6C8
extern "C++" void* gScreenSets;       // 0x805FB654

static const u32 kHashBack = 0x5B5B2C83u;   // "back"
static const u32 kHashTimer = 0x9D841CAAu;  // "timer"

extern "C" {

// Creates the voice SFX handle on first use (fn_801D40E0, then fn_80209814 if that returned non-NULL).
static void EnsureVoiceSfx(void) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
}

// Applies one rotation step to the element at +0x18C of a wobble slot: the z rotation (+0x98)
// changes by +/-0.0872665 (5 degrees) depending on the sign of the slot's step at +0x198; x/y get +0.
static void WobbleStep(u8* p) {
    s32 step = *(s32*)(p + 0x198);
    f32 d = lbl_805FC9D8;
    if (step < 0) {
        d = d * lbl_805FC9DC;
    } else if (step == 0) {
        d = lbl_805FC990;
    }
    u8* el = *(u8**)(p + 0x18C);
    f32 x = *(f32*)(el + 0x90) + lbl_805FC990;
    f32 z = *(f32*)(el + 0x98);
    f32 y = *(f32*)(el + 0x94) + lbl_805FC990;
    f32 r = z + d;
    *(f32*)(el + 0x90) = x;
    *(f32*)(el + 0x94) = y;
    *(f32*)(el + 0x98) = r;
    el = *(u8**)(p + 0x18C);
    (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
}

// 0x800267FC
// Per-frame update of the four wobbling elements (slots of 0x14 bytes from +0x190): a slot with
// no timer picks a new random angle step and duration; while steps remain it rotates the element
// by 5 degrees per step; afterwards it drifts the element on a circle (0.7 radius), clamped to the
// 640x480 screen. Then runs the base update and the object's virtual update (slot 0x6C).
void fn_800267FC(u8* arg0, f32 farg0) {
    u8* p = arg0;
    for (s32 i = 0; i < 4; i++, p += 0x14) {
        if (*(s32*)(p + 0x190) == 0) {
            s32 n = (s32)(lbl_805FC9C8 + lbl_805FC9CC * (lbl_805FC9C4 * (f32)RandomU32()));
            s32 step = (n / 5) * 5;
            s32 count = __cvt_fp2signed(step) / 5;
            s32 c = 1;
            if (count > 1) c = count;
            *(s32*)(p + 0x19C) = c;
            *(s32*)(p + 0x198) = step;
            *(s32*)(p + 0x190) = (s32)(lbl_805FC9D0 + lbl_805FC9D4 * (lbl_805FC9C4 * (f32)RandomU32()));
            *(s32*)(p + 0x194) = *(s32*)(p + 0x194) + step;
            WobbleStep(p);
            *(s32*)(p + 0x19C) = *(s32*)(p + 0x19C) - 1;
        } else if (*(s32*)(p + 0x19C) > 0) {
            WobbleStep(p);
            *(s32*)(p + 0x19C) = *(s32*)(p + 0x19C) - 1;
        } else {
            f32 ang = lbl_805FC9E0 * (f32)*(s32*)(p + 0x194);
            f32 dy = lbl_805FC9E4 * sinf(ang);
            f32 dx = lbl_805FC9E4 * cosf(ang);
            u8* el = *(u8**)(p + 0x18C);
            f32 x = *(f32*)(el + 0x68) + dx;
            f32 y = *(f32*)(el + 0x6C) + dy;
            f32 z = *(f32*)(el + 0x70) + lbl_805FC990;
            if (x > lbl_805FC9E8) {
                x = lbl_805FC9E8;
            } else if (x < lbl_805FC990) {
                x = lbl_805FC990;
            }
            if (y > lbl_805FC9EC) {
                y = lbl_805FC9EC;
            } else if (y < lbl_805FC990) {
                y = lbl_805FC990;
            }
            *(f32*)(el + 0x68) = x;
            *(f32*)(el + 0x6C) = y;
            *(f32*)(el + 0x70) = z;
            el = *(u8**)(p + 0x18C);
            (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 0x18);
            *(s32*)(p + 0x190) = *(s32*)(p + 0x190) - 1;
        }
    }
    CatchMinigame_UpdateBase(arg0, farg0);
    (*(ObjFn*)(*(u8**)(arg0 + 0x4) + 0x6C))(arg0);
}

// 0x80028340
// One-time init of the catch screen: binds the "timer" and "back" elements, reads the marker
// positions ("Marker_%d" into the box at +0xF8), creates the hand-shadow object (+0xA0), builds
// the bug lanes ("bugS%d"/"bugSplat%d"/"bug%d"/"bug%dA", stride 0x128, array at +0xA4, count +0xC0)
// and fly lanes ("flyS%d"/"flySplat%d"/"fly%d", stride 0x12C, array at +0xA8, count +0xC4), the
// "leftmeter" bar (+0x98), then sends event 0xF15918A6 to the target at +0x90. Runs once (flag +0x11C).
void fn_80028340(u8* arg0) {
    fn_80021ECC();
    if (*(s32*)(arg0 + 0x11C) != 0) return;

    char name[11];
    char buf[256];

    fn_800223F0(arg0, (f32)*(s32*)(arg0 + 0xD8));
    void* set = ScreenSetsCurrent(gScreenSets);
    u8* back = (u8*)ScreenSetFindElement(set, kHashBack);
    *(f32*)(arg0 + 0xDC) = *(f32*)(back + 0x68);
    *(f32*)(arg0 + 0xE0) = *(f32*)(back + 0x6C);
    *(f32*)(arg0 + 0xE4) = *(f32*)(back + 0x68) + *(f32*)(back + 0x74);
    *(f32*)(arg0 + 0xE8) = *(f32*)(back + 0x6C) + *(f32*)(back + 0x78);
    for (s32 i = 0; i < 0xE; i++) {
        SysSprintf(name, "Marker_%d", i);
        u8* m = (u8*)ScreenSetFindElement(set, StringHash(name));
        fn_800311E8(*(u8**)(arg0 + 0xF8), (f32*)(m + 0x68));
    }
    fn_8002A8CC();
    u8* shadow = (u8*)fn_801D40E0(0x5BC);
    if (shadow != NULL) {
        void* el = ScreenSetFindElement(set, StringHash("hand_shadow"));
        shadow = fn_8002B58C(shadow, el, *(s32*)(arg0 + 0xC8), arg0 + 0xDC, arg0 + 0xE4, *(u8**)(arg0 + 0xF8));
    }
    *(u8**)(arg0 + 0xA0) = shadow;
    fn_8002BD60(shadow);

    u8* lane;
    s32 off = 0;
    for (s32 i = 0; i < *(s32*)(arg0 + 0xC0); i++) {
        SysSprintf(buf, "bugS%d", i);
        void* e1 = ScreenSetFindElement(set, StringHash(buf));
        SysSprintf(buf, "bugSplat%d", i);
        void* e2 = ScreenSetFindElement(set, StringHash(buf));
        SysSprintf(buf, "bug%d", i);
        void* e0 = ScreenSetFindElement(set, StringHash(buf));
        lane = *(u8**)(arg0 + 0xA4) + off;
        fn_800294BC(lane, e0, e1, e2, *(s32*)(arg0 + 0xC8));
        SysSprintf(buf, "bug%dA", i);
        void* ea = ScreenSetFindElement(set, StringHash(buf));
        fn_8002A7F8(*(u8**)(arg0 + 0xA4) + off, ea);
        fn_800296C4(*(u8**)(arg0 + 0xA4) + off, arg0 + 0xDC, arg0 + 0xE4, *(u8**)(arg0 + 0xF8));
        fn_8002A178(*(u8**)(arg0 + 0xA4) + off, *(void**)(arg0 + 0xA0));
        lane = *(u8**)(arg0 + 0xA4) + off;
        (*(ObjFn*)(*(u8**)lane + 0x10))(lane);
        off += 0x128;
    }

    off = 0;
    for (s32 i = 0; i < *(s32*)(arg0 + 0xC4); i++) {
        SysSprintf(buf, "flyS%d", i);
        void* e1 = ScreenSetFindElement(set, StringHash(buf));
        SysSprintf(buf, "flySplat%d", i);
        void* e2 = ScreenSetFindElement(set, StringHash(buf));
        SysSprintf(buf, "fly%d", i);
        u32 h = StringHash(buf);
        void* e0 = ScreenSetFindElement(set, h);
        u8* fly = (u8*)ScreenSetFindElement(set, h);
        fn_800294BC(*(u8**)(arg0 + 0xA8) + off, e0, e1, e2, *(s32*)(arg0 + 0xC8));
        *(f32*)(fly + 0x154) = lbl_805FCA44;
        (*(ElemFn*)(*(u8**)(fly + 0x138) + 0x2C))(fly, 2);
        fn_800296C4(*(u8**)(arg0 + 0xA8) + off, arg0 + 0xDC, arg0 + 0xE4, *(u8**)(arg0 + 0xF8));
        fn_8002A178(*(u8**)(arg0 + 0xA8) + off, *(void**)(arg0 + 0xA0));
        lane = *(u8**)(arg0 + 0xA8) + off;
        (*(ObjFn*)(*(u8**)lane + 0x10))(lane);
        off += 0x12C;
    }

    CatchMinigame_UpdateBase(arg0, lbl_805FCA38);
    if (*(u8*)(arg0 + 0xFC) == 0 && (s32)*(f32*)(arg0 + 0x6C) == 10 &&
        (f64)((f32)*(s32*)(arg0 + 0xB4) / (f32)*(s32*)(arg0 + 0xCC)) < lbl_805FCA48) {
        *(u8*)(arg0 + 0xFC) = 1;
        fn_800225C8(arg0);
    }

    u8* meter = (u8*)ScreenSetFindElement(set, StringHash("leftmeter"));
    *(u8**)(arg0 + 0x98) = meter;
    f32 full = *(f32*)(meter + 0x74);
    *(f32*)(arg0 + 0x9C) = full;
    f32 w = (full * (f32)*(s32*)(arg0 + 0xB4)) / (f32)*(s32*)(arg0 + 0xCC);
    if (!(w < full)) w = full;
    *(f32*)(*(u8**)(arg0 + 0x98) + 0x74) = w;
    meter = *(u8**)(arg0 + 0x98);
    (*(ElemFn*)(*(u8**)(meter + 0x138) + 0x2C))(meter, 0x1A);

    u8* bk = (u8*)ScreenSetFindElement(set, StringHash("back"));
    *(u8**)(arg0 + 0x104) = bk;
    *(f32*)(arg0 + 0x108) = *(f32*)(bk + 0x68);
    *(f32*)(arg0 + 0x10C) = *(f32*)(bk + 0x6C);
    *(f32*)(arg0 + 0x110) = *(f32*)(bk + 0x70);
    lbl_805FAFC4 = 0;
    EventSend(gEventMgr, *(void**)(arg0 + 0x90), NULL, 0xF15918A6u, 0, 0, 1, 0);
    *(s32*)(arg0 + 0x11C) = 1;
}

// 0x80034B98
// Per-frame update of the tomato-throw minigame: timers, the loop sound, the 5-state sprite
// selector (+0x17C, ping-pongs 0..4 while active), power meter (+0x16C), the four lanes
// (fn_80035164 with team bits 1/2/4/8), the progress bar (+0x160), the "out of time" sound, and the
// end-of-game checks that signal GameFlow.
void fn_80034B98(u8* arg0, f32 farg0) {
    fn_80036618(arg0, farg0);

    s32 wrapped = 0;
    f32 t = *(f32*)(arg0 + 0x180) + farg0;
    f32 period = *(f32*)(arg0 + 0x184);
    *(f32*)(arg0 + 0x180) = t;
    if (t >= period) {
        wrapped = 1;
        *(f32*)(arg0 + 0x180) = t - period;
    }
    if (*(s32*)(arg0 + 0x630) > -1) {
        *(f32*)(arg0 + 0x62C) = *(f32*)(arg0 + 0x62C) - farg0;
    }

    s32 active = (*(s32*)(arg0 + 0x624) == 0);
    if (active) {
        if (*(u8*)(arg0 + 0x120) != 0) {
            f32 v = *(f32*)(arg0 + 0x124);
            if (v > lbl_805FCDBC) *(f32*)(arg0 + 0x98) = v;
        }
        active = *(u8*)(arg0 + 0x120);
    }
    if (active != 0) {
        EnsureVoiceSfx();
        if (fn_8020A9A8(*(s32*)(arg0 + 0x630)) == 0) {
            *(f32*)(arg0 + 0x62C) = lbl_805FCDE0;
            EnsureVoiceSfx();
            *(s32*)(arg0 + 0x630) = PlaySound(0x4229832Du, 0, 0, 0, 0, 0);
        }
    }

    if (*(s32*)(arg0 + 0x17C) == 2) {
        *(u8*)(arg0 + 0x168) = 0;
    } else {
        *(u8*)(arg0 + 0x168) = 1;
    }
    if (active != 0 || *(u8*)(arg0 + 0x168) != 0) {
        if (lbl_805F8484 <= lbl_805FCDBC) {
            s32 st = *(s32*)(arg0 + 0x17C);
            lbl_805F8484 = lbl_805FCE58;
            if (st <= 0) {
                lbl_805F8480 = 1;
            } else if (st == 4) {
                lbl_805F8480 = -1;
            }
            *(s32*)(arg0 + 0x17C) = *(s32*)(arg0 + 0x17C) + lbl_805F8480;
        } else {
            lbl_805F8484 = lbl_805F8484 - farg0;
        }
    } else {
        *(u8*)(arg0 + 0x168) = 0;
        if (wrapped == 0) *(s32*)(arg0 + 0x17C) = 2;
    }

    s32 sel = *(s32*)(arg0 + 0x17C);
    ScreenSetsCurrent(gScreenSets);
    for (s32 i = 0; i < 5; i++) {
        u8* el = *(u8**)(arg0 + 0x14C + i * 4);
        if (i == sel) {
            (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
        } else {
            (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        }
    }
    fn_80035DB8(arg0, farg0);

    if (*(u8*)(arg0 + 0x168) != 0) {
        f32 cur = *(f32*)(arg0 + 0x16C);
        f32 maxv = *(f32*)(arg0 + 0x170);
        if (cur < maxv) {
            *(f32*)(arg0 + 0x16C) = cur + *(f32*)(arg0 + 0x174);
        } else {
            *(f32*)(arg0 + 0x16C) = maxv;
        }
        lbl_805FAFF8 = lbl_805FAFF8 + farg0;
        while (lbl_805FAFF8 > lbl_805FCDBC) {
            s32 n = (s32)*(f32*)(arg0 + 0x16C);
            lbl_805FAFF8 = lbl_805FAFF8 - lbl_805FCE5C;
            fn_80035A04(arg0, n);
        }
    } else {
        f32 cur = *(f32*)(arg0 + 0x16C);
        if (cur > lbl_805FCDBC) {
            *(f32*)(arg0 + 0x16C) = cur - *(f32*)(arg0 + 0x174);
        } else {
            *(f32*)(arg0 + 0x16C) = lbl_805FCDBC;
        }
    }

    fn_80035164(arg0, 1, farg0);
    fn_80035164(arg0, 2, farg0);
    fn_80035164(arg0, 4, farg0);
    fn_80035164(arg0, 8, farg0);

    if (*(s32*)(arg0 + 0x624) == 0) {
        s32 sum = *(s32*)(arg0 + 0x424) + *(s32*)(arg0 + 0x254) + *(s32*)(arg0 + 0x364);
        f32 full = *(f32*)(arg0 + 0x164);
        f32 w = (full * (f32)sum) / (f32)*(s32*)(arg0 + 0x5F4);
        if (w > full) w = full;
        f32 px = *(f32*)(arg0 + 0x5F8);
        f32 py = *(f32*)(arg0 + 0x5FC);
        f32 pz = *(f32*)(arg0 + 0x600);
        u8* bar = *(u8**)(arg0 + 0x160);
        *(f32*)(bar + 0x68) = px;
        *(f32*)(bar + 0x6C) = py;
        *(f32*)(bar + 0x70) = pz;
        *(f32*)(*(u8**)(arg0 + 0x160) + 0x74) = w;
        bar = *(u8**)(arg0 + 0x160);
        (*(ElemFn*)(*(u8**)(bar + 0x138) + 0x2C))(bar, 0x1A);
        CatchMinigame_UpdateBase(arg0, farg0);
        if (*(f32*)(arg0 + 0x6C) <= (f32)lbl_805F8488) {
            EnsureVoiceSfx();
            PlaySound(0xDD51E007u, 0, 0, 0, 0, 0);
            lbl_805F8488 = lbl_805F8488 - 1;
            if (lbl_805F8488 < 0) lbl_805F8488 = 4;
        }
    }

    s32 result = 0;
    if (*(s32*)(arg0 + 0x624) == 0 && fn_8003639C(arg0) != 0) result = 1;
    // The asm has a switch on result (1 -> 2, 2 -> 1, 4 -> 0, otherwise 2), but result is only 0 or 1
    // here, so only "result != 0 -> 2" is reachable.
    if (result != 0) {
        *(s32*)((u8*)gLab + 0x60) = 2;
        StopRumble(*(s32*)(lbl_803C4298 + 0x38));
        UIElement_SetVisibleFlag(kHashTimer, 0);
        UIElement_SetVisibleFlag(0x0CAF9C6Fu, 0);
        GameFlowSignal((s32)0xC608C842u, 0, 0);
    }

    if (*(f32*)(arg0 + 0x6C) <= lbl_805FCDBC) {
        StopRumble(*(s32*)(lbl_803C4298 + 0x38));
        GameFlowSignal(0x117741C3, 0, 0);
    }
    (*(ObjFn*)(*(u8**)(arg0 + 0x4) + 0x6C))(arg0);
}

// 0x80035DB8
// Moves the player's marker (+0x59C) from the stick: horizontal speed (+0x598) ramps up while the
// stick is held and decays when released (+0x590 deceleration, +0x5B8 while the hit state +0x5AC is
// active), x is kept in 114..526, y follows an arc over the hit-state timer (+0x5B4/+0x5B0), the
// z rotation (+0x98) is eased towards the target tilt, and the "hit the ground" element (+0x608) is
// placed and a sound played the first time the marker reaches the ground line.
void fn_80035DB8(u8* arg0, f32 farg0) {
    char text[36];
    f32 stickX = lbl_805FCDBC;
    f32 stickY = lbl_805FCDBC;
    f32 tilt;       // f29
    f32 phase;      // f30
    f32 dirn;       // f31
    f32 arcT;       // f4

    f32 timer = *(f32*)(arg0 + 0x5B4) + farg0;
    *(f32*)(arg0 + 0x5B4) = timer;
    if (timer > *(f32*)(arg0 + 0x5B0)) {
        *(u8*)(arg0 + 0x5AC) = 0;
        *(f32*)(arg0 + 0x5B4) = lbl_805FCDBC;
    }

    u8* marker = *(u8**)(arg0 + 0x59C);
    *(f32*)(arg0 + 0x588) = *(f32*)(marker + 0x68);
    *(f32*)(arg0 + 0x58C) = *(f32*)(marker + 0x6C);
    if (*(s32*)(arg0 + 0x624) == 0) {
        StickRead(*(s32*)(lbl_803C4298 + 0x38) * 0x10 + 0xE, &stickX, &stickY);
    }
    if (stickX <= lbl_805FCE9C) {
        if (stickX >= lbl_805FCEA0) stickX = lbl_805FCDBC;
    }

    f32 mag = stickX;
    if (mag < lbl_805FCDBC) {
        mag = -mag;
        *(s32*)(arg0 + 0x620) = 0;
        dirn = lbl_805FCE20;
    } else if (mag == lbl_805FCDBC) {
        dirn = lbl_805FAFFC;
    } else {
        *(s32*)(arg0 + 0x620) = 0;
        dirn = lbl_805FCE14;
    }
    if (dirn != lbl_805FAFFC) {
        EnsureVoiceSfx();
        PlaySound(0x72355C38u, 0, 0, 0, 0, 0);
    }

    u8 hitActive = *(u8*)(arg0 + 0x5AC);
    if (hitActive != 0) {
        *(f32*)(arg0 + 0x598) = *(f32*)(arg0 + 0x598) - *(f32*)(arg0 + 0x5B8) * farg0;
    } else if (lbl_805FCDBC == stickX) {
        *(f32*)(arg0 + 0x598) = *(f32*)(arg0 + 0x598) - *(f32*)(arg0 + 0x590) * farg0;
    } else {
        f32 spd = *(f32*)(arg0 + 0x598);
        f32 maxSpd = *(f32*)(arg0 + 0x594);
        // (the asm re-tests hitActive != 0 here; it is always 0 on this path)
        if (spd < maxSpd) {
            *(f32*)(arg0 + 0x598) = spd + farg0 * (mag * *(f32*)(arg0 + 0x590));
        }
    }
    f32 spd = *(f32*)(arg0 + 0x598);
    f32 maxSpd = *(f32*)(arg0 + 0x594);
    if (spd > maxSpd) {
        *(f32*)(arg0 + 0x598) = maxSpd;
    } else if (spd <= lbl_805FCDBC) {
        dirn = lbl_805FCDBC;
        *(f32*)(arg0 + 0x598) = lbl_805FCDBC;
        lbl_805FAFFC = lbl_805FCDBC;
    }

    if (lbl_805FB000 % 30 == 0) {
        SysSprintf(text, "Last frame time: %f\n", (f64)farg0);
        // Print is a script callback declared with a u32 argument in script_callbacks.h; the asm
        // passes the buffer pointer, so it takes a string pointer.
        Print(text);
    }

    f32 px = *(f32*)(arg0 + 0x588);
    if (px > lbl_805FCDD0 && px < lbl_805FCEA4) {
        *(f32*)(arg0 + 0x588) = px + farg0 * (dirn * *(f32*)(arg0 + 0x598));
    }
    px = *(f32*)(arg0 + 0x588);
    if (px <= lbl_805FCDD0) {
        *(f32*)(arg0 + 0x588) = lbl_805FCEA8;
    } else if (px > lbl_805FCEA4) {
        *(f32*)(arg0 + 0x588) = lbl_805FCEAC;
    }

    hitActive = *(u8*)(arg0 + 0x5AC);
    f32 hitLen = *(f32*)(arg0 + 0x5B0);
    tilt = *(f32*)(arg0 + 0x5C0);
    f32 quarter = lbl_805FCE90 * hitLen;
    if (hitActive != 0) {
        f32 ht = *(f32*)(arg0 + 0x5B4);
        tilt = *(f32*)(arg0 + 0x5BC);
        if (ht <= quarter) {
            if (lbl_805FB004 == lbl_805FCDBC) {
                if (dirn == lbl_805FCDBC) {
                    f32 z = *(f32*)(*(u8**)(arg0 + 0x59C) + 0x98);
                    lbl_805FB004 = lbl_805FCE14;
                    if (z < lbl_805FCDBC) z = -z;  // fabs
                    if (z <= lbl_805FCEB0) {
                        f32 pos[3];  // scratch vector (uninitialised in the original)
                        fn_800359F4((u8*)pos, lbl_805FCDBC, lbl_805FCDBC, lbl_805FCDBC);
                    }
                } else {
                    lbl_805FB004 = dirn;
                }
                *(f32*)(arg0 + 0x598) = *(f32*)(arg0 + 0x594);
            }
            phase = lbl_805FCDE0;
            arcT = *(f32*)(arg0 + 0x5B4) / quarter;
        } else {
            f32 tail = hitLen - quarter;
            if (ht >= tail) {
                phase = lbl_805FCDE0;
                tilt = lbl_805FCDBC;
                arcT = lbl_805FCE14 - (ht - tail) / quarter;
            } else {
                phase = lbl_805FCE14;
                arcT = phase;
            }
        }
        lbl_805FAFFC = lbl_805FB004;
    } else {
        if (lbl_805FCDBC != lbl_805FB004) lbl_805FB004 = lbl_805FCDBC;
        arcT = lbl_805FCDBC;
        phase = *(f32*)(arg0 + 0x598) / *(f32*)(arg0 + 0x594);
    }

    f32 ratio = *(f32*)(arg0 + 0x598) / *(f32*)(arg0 + 0x594);
    f32 amp = *(f32*)(arg0 + 0x5E4);
    f32 groundY = *(f32*)(arg0 + 0x5C8);
    f32 z = *(f32*)(marker + 0x70);
    f32 mx = *(f32*)(arg0 + 0x588);
    f32 baseY = *(f32*)(arg0 + 0x5CC);
    f32 one = lbl_805FCE14;
    f32 lift = arcT * amp;
    f32 peakY = (groundY - amp) + lbl_805FCDE0 * lift;
    *(f32*)(marker + 0x68) = mx;
    f32 my = baseY + (one - ratio) * (peakY - baseY);
    f32 groundLine = peakY - one;
    *(f32*)(marker + 0x6C) = my;
    *(f32*)(marker + 0x70) = z;

    u8* m2 = *(u8**)(arg0 + 0x59C);
    if (*(f32*)(m2 + 0x6C) > groundLine && *(s32*)(arg0 + 0x620) == 0) {
        u8* fx = *(u8**)(arg0 + 0x608);
        *(s32*)(arg0 + 0x620) = 1;
        f32 gy = *(f32*)(arg0 + 0x5C8);
        f32 gx = *(f32*)(m2 + 0x68);
        f32 gz = *(f32*)(fx + 0x70);
        fn_801A8654(fx, 1);
        fx = *(u8**)(arg0 + 0x608);
        *(f32*)(fx + 0x68) = gx;
        *(f32*)(fx + 0x6C) = gy;
        *(f32*)(fx + 0x70) = gz;
        fx = *(u8**)(arg0 + 0x608);
        (*(ElemFn*)(*(u8**)(fx + 0x138) + 0x2C))(fx, 0x1A);
        EnsureVoiceSfx();
        PlaySound(0xCEAF7619u, 0, 0, 0, 0, 0);
    }

    f32 target = dirn * (tilt * phase);
    u8* tm = *(u8**)(arg0 + 0x59C);
    f32 curZ = *(f32*)(tm + 0x98);
    f32 newZ;
    if (*(u8*)(arg0 + 0x5AC) != 0) {
        newZ = curZ + phase * ((tilt * lbl_805FB004) - curZ);
    } else {
        newZ = curZ + farg0 * (*(f32*)(arg0 + 0x5C4) * (target - curZ));
    }
    if (lbl_805FCDBC != tilt) {
        if (newZ < lbl_805FCDBC) {
            f32 lo = -tilt;
            if (newZ < lo) newZ = lo;
        } else if (newZ > tilt) {
            newZ = tilt;
        }
    }
    *(f32*)(tm + 0x98) = newZ;
    tm = *(u8**)(arg0 + 0x59C);
    (*(ElemFn*)(*(u8**)(tm + 0x138) + 0x2C))(tm, 0x18);
    lbl_805FAFFC = dirn;
}

// 0x8003A2B4
// Setup of the slap minigame screen: tuning constants (+0x378..+0x3C0, +0x954/958), the seven
// lane records via fn_8003CE20, per-slot flags (+0xBC/+0xBE, lanes 5 and 6 inactive) and their
// element visibility, the fixed elements (+0x25C..+0x278 by hash, fx_punch/fx_slap/fx_hit), the
// six Tomato/TomatoSplat/TomatoHighlight/Pipe/Highlight element sets, the Monster* hand elements and
// textures, the Impact elements, the cleared 6-byte flags and the "ShowMeter" element (+0x9B4), whose
// original position is saved the first time and which is then moved to x = 700 with width 0.
void fn_8003A2B4(u8* arg0) {
    char buf[32];

    *(f32*)(arg0 + 0x378) = lbl_805FCF98;
    *(f32*)(arg0 + 0x37C) = lbl_805FCF9C;
    *(f32*)(arg0 + 0x380) = lbl_805FCFA0;
    *(f32*)(arg0 + 0x384) = lbl_805FCFA4;
    *(f32*)(arg0 + 0x388) = lbl_805FCFC8;
    *(s32*)(arg0 + 0x3A8) = 6;
    *(f32*)(arg0 + 0x3AC) = lbl_805FCFCC;
    *(f32*)(arg0 + 0x394) = lbl_805FCFD0;
    *(f32*)(arg0 + 0x954) = lbl_805FCFB8;
    *(s32*)(arg0 + 0x3BC) = 4;
    *(s32*)(arg0 + 0x3C0) = 2;
    *(f32*)(arg0 + 0x958) = lbl_805FCFAC;

    void* set = ScreenSetsCurrent(gScreenSets);
    fn_8003CE20(arg0 + 0x90, 0, 0);
    fn_8003CE20(arg0 + 0xC4, 0, 1);
    fn_8003CE20(arg0 + 0xF8, 1, 0);
    fn_8003CE20(arg0 + 0x12C, 2, 0);
    fn_8003CE20(arg0 + 0x160, 2, 1);
    fn_8003CE20(arg0 + 0x194, 3, 0);
    fn_8003CE20(arg0 + 0x1C8, 3, 1);

    *(s32*)(arg0 + 0x3B8) = 0;
    for (s32 i = 0; i < 7; i++) {
        u8* slot = arg0 + i * 0x34;
        u8 off = (u8)(i >= 5);
        *(u8*)(slot + 0xBE) = off;
        *(u8*)(slot + 0xBC) = off;
        u8* el = *(u8**)(slot + 0x98);
        if (off == 0) {
            *(s32*)(arg0 + 0x3B8) = *(s32*)(arg0 + 0x3B8) + 1;
            (*(ElemFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
        } else {
            (*(ElemFn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
        }
    }
    *(s32*)(arg0 + 0x984) = *(s32*)(arg0 + 0x3B8);

    *(void**)(arg0 + 0x25C) = ScreenSetFindElement(set, 0x22CE8C5Au);
    *(void**)(arg0 + 0x260) = ScreenSetFindElement(set, 0x55C9BCCCu);
    *(void**)(arg0 + 0x264) = ScreenSetFindElement(set, 0xCCC0ED76u);
    *(void**)(arg0 + 0x268) = ScreenSetFindElement(set, 0xBBC7DDE0u);
    *(void**)(arg0 + 0x26C) = ScreenSetFindElement(set, 0x25A34843u);
    *(void**)(arg0 + 0x270) = ScreenSetFindElement(set, 0x52A478D5u);
    *(void**)(arg0 + 0x274) = ScreenSetFindElement(set, 0xCBAD296Fu);
    *(void**)(arg0 + 0x278) = ScreenSetFindElement(set, 0xBCAA19F9u);
    *(void**)(arg0 + 0x944) = ScreenSetFindElement(set, StringHash("fx_punch"));
    *(void**)(arg0 + 0x948) = ScreenSetFindElement(set, StringHash("fx_slap"));
    *(void**)(arg0 + 0x94C) = ScreenSetFindElement(set, StringHash("fx_hit"));

    for (s32 i = 0; i < 6; i++) {
        u8* slot = arg0 + i * 4;
        SysSprintf(buf, "Tomato_%d", i);
        *(void**)(slot + 0x1FC) = ScreenSetFindElement(set, StringHash(buf));
        SysSprintf(buf, "TomatoSplat_%d", i);
        *(void**)(slot + 0x214) = ScreenSetFindElement(set, StringHash(buf));
        SysSprintf(buf, "TomatoHighlight_%d", i);
        *(void**)(slot + 0x22C) = ScreenSetFindElement(set, StringHash(buf));
        SysSprintf(buf, "Pipe_%d", i);
        *(void**)(slot + 0x244) = ScreenSetFindElement(set, StringHash(buf));
        SysSprintf(buf, "Highlight_%d", i);
        *(void**)(slot + 0x27C) = ScreenSetFindElement(set, StringHash(buf));
    }

    fn_801B3310(fn_801B3308(), arg0 + 0x3E4);
    u8* hand = (u8*)ScreenSetFindElement(set, StringHash("MonsterHand"));
    *(u8**)(arg0 + 0x924) = hand;
    *(void**)(arg0 + 0x3C8) = (void*)fn_801A1208(hand);
    *(void**)(arg0 + 0x3CC) = (void*)fn_801A1208((u8*)ScreenSetFindElement(set, StringHash("MonsterPunch")));
    *(void**)(arg0 + 0x3D0) = (void*)fn_801A1208((u8*)ScreenSetFindElement(set, StringHash("MonsterSlap")));
    *(void**)(arg0 + 0x3D4) = (void*)fn_801A1208((u8*)ScreenSetFindElement(set, StringHash("MonsterSlapFlip")));
    *(void**)(arg0 + 0x928) = ScreenSetFindElement(set, StringHash("MonsterHandInvis"));
    *(void**)(arg0 + 0x92C) = fn_80198F1C(set, StringHash("Impact"));
    *(void**)(arg0 + 0x930) = ScreenSetFindElement(set, StringHash("ImpactScanlines"));
    *(void**)(arg0 + 0x934) = fn_80198F1C(set, StringHash("MonsterHandWaveLeft"));
    *(void**)(arg0 + 0x938) = fn_80198F1C(set, StringHash("MonsterHandWaveRight"));
    *(void**)(arg0 + 0x93C) = fn_80198F1C(set, StringHash("MonsterHandPunch"));
    *(void**)(arg0 + 0x940) = fn_80198F1C(set, StringHash("MonsterHandReset"));
    memset(arg0 + 0x294, 0, 6);
    memset(arg0 + 0x29A, 0, 6);

    u8* meter = (u8*)ScreenSetFindElement(set, StringHash("ShowMeter"));
    *(u8**)(arg0 + 0x9B4) = meter;
    // Save the meter's original position the first time (while the saved x/y/z are all 0).
    if (*(f32*)(arg0 + 0x9BC) == lbl_805FCFAC && *(f32*)(arg0 + 0x9C0) == lbl_805FCFAC &&
        *(f32*)(arg0 + 0x9C4) == lbl_805FCFAC) {
        *(f32*)(arg0 + 0x9BC) = *(f32*)(meter + 0x68);
        *(f32*)(arg0 + 0x9C0) = *(f32*)(meter + 0x6C);
        *(f32*)(arg0 + 0x9C4) = *(f32*)(meter + 0x70);
        *(f32*)(arg0 + 0x9B8) = *(f32*)(meter + 0x74);
    }
    *(f32*)(*(u8**)(arg0 + 0x9B4) + 0x74) = lbl_805FCFAC;
    *(f32*)(*(u8**)(arg0 + 0x9B4) + 0x68) = lbl_805FCFD4;
    meter = *(u8**)(arg0 + 0x9B4);
    (*(ElemFn*)(*(u8**)(meter + 0x138) + 0x2C))(meter, 0x18);
}

}  // extern "C"
