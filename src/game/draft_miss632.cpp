// Mixed game functions (0x8007D004 - 0x800AEFEC): damage/score tally, experiment-screen meters,
// widget setup, ray-scene actor updates and a sphere sweep test.
#include "types.h"
#include <math.h>

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
u32 RandomU32(void);                                       // 0x80143460
s32 RaySceneTest(void* ray);                               // 0x80070458 (asm takes no argument)
void* ScreenSetFindElement(void* set, u32 hash);           // 0x80198F18
void* ScreenSetsCurrent(void* sets);                       // 0x8018D1E4
void StickRead(s32 channel, f32* x, f32* y);               // 0x80147704
void UISetProgress(void* obj, f32 t);
void* UIWidgetFindChild(void* widget, u32 hash);           // 0x80199AEC
void UIWidgetPlayAnimLoop(void* widget, u32 anim, s32 count);  // 0x8019BB1C
void* fn_80070F4C(void);                                   // 0x80070F4C
void fn_80070FAC(u8* p, f32 v);                            // 0x80070FAC
void fn_800712B4(void);                                    // 0x800712B4
void fn_800712B8(void);                                    // 0x800712B8
void fn_800712BC(u8* p, s32 a, s32 b);                     // 0x800712BC
void fn_800715B8(u8* p, s32 a);                            // 0x800715B8
void fn_80071838(u8* arg0, u32 arg1);                      // 0x80071838
u8* fn_80078BB0(void);                                     // 0x80078BB0 (returns a pointer; leaf has s32)
f32 fn_8007C138(u8* a3);                                   // 0x8007C138
void fn_8009E2AC(u8* arg0, s32 arg1);                      // 0x8009E2AC
void fn_800A9B9C(u8* self, f32 dt);                        // 0x800A9B9C
void fn_800AA094(u8* self, f32 dt);                        // 0x800AA094
void fn_800AA2D0(u8* self, f32 dt, f32 v);                 // 0x800AA2D0
u32 fn_800AA9D4(u8* self, u8* obj);                        // 0x800AA9D4
s32 fn_800AF3E0(u8* self, u8* pos, u8* dir);               // 0x800AF3E0
u8* fn_800ABD80(u8* arg0);                                 // 0x800ABD80
s32 fn_800B0530(u8* p);                                    // 0x800B0530
s32 fn_800B058C(u8* p, void** out, s32 max);               // 0x800B058C
s32 fn_800B13B8(u8* p);                                    // 0x800B13B8
void fn_800C324C(u8* a, u8* b, f32 c);                     // 0x800C324C
f32 fn_800C3E84(u8* a);                                    // 0x800C3E84
void fn_800C597C(u8* outQuat, u8* pos, f32 t);             // 0x800C597C
u8* fn_8015A210(u8* p, s32 a, s32* out, s32 b);            // 0x8015A210
s32 fn_8019BC80(u8* arg0, u32 arg1);                       // 0x8019BC80
s32 fn_801A403C(u8* a3, f32 f1);                           // 0x801A403C
s32 fn_801A4050(u8* a3, f32 f1);                           // 0x801A4050
s32 fn_801A4078(u8* a3, f32 f1);                           // 0x801A4078
f32 fn_801BFE38(u8* a, f32* b, u8* c, u8* d, f32* outT0, f32* outT1);  // 0x801BFE38

extern f32 lbl_80346378[];     // 0x80346378 (float table indexed by RaySceneTest)
extern f32 lbl_80346388[];     // 0x80346388 (float table indexed by RaySceneTest)
extern u32 lbl_80347D60[];     // 0x80347D60 (5 element hashes, experiment screen)
extern f32 lbl_803474C8[];     // 0x803474C8 (3 floats: meter colour r,g,b)
extern u8 lbl_803C4298[];      // 0x803C4298
extern f32 lbl_805FD748;       // 0x805FD748 (0.0)
extern f32 lbl_805FD750;       // 0x805FD750 (100.0)
extern f32 lbl_805FD760;       // 0x805FD760 (1.0)
extern f32 lbl_805FD764;       // 0x805FD764 (score multiplier)
extern f32 lbl_805FD768;       // 0x805FD768 (score multiplier)
extern f32 lbl_805FD76C;       // 0x805FD76C (meter scale)
extern f32 lbl_805FD928;       // 0x805FD928 (0.0)
extern f32 lbl_805FD934;       // 0x805FD934 (1.0)
extern f32 lbl_805FD940;       // 0x805FD940
extern f32 lbl_805FD944;       // 0x805FD944 (100.0)
extern f32 lbl_805FD948;       // 0x805FD948
extern f32 lbl_805FD94C;       // 0x805FD94C
extern f32 lbl_805FD950;       // 0x805FD950
extern f32 lbl_805FDC50;       // 0x805FDC50 (0.0)
extern f32 lbl_805FDC5C;       // 0x805FDC5C (0.5)
extern f32 lbl_805FDC7C;       // 0x805FDC7C (1 / 2^32)
extern f32 lbl_805FDC80;       // 0x805FDC80
extern f32 lbl_805FDF18;       // 0x805FDF18 (0.0)
extern f32 lbl_805FDF48;       // 0x805FDF48 (10.0)
extern f32 lbl_805FDF54;       // 0x805FDF54
extern f32 lbl_805FDF58;       // 0x805FDF58
extern f32 lbl_805FDF5C;       // 0x805FDF5C (100.0)
extern f32 lbl_805FE038;       // 0x805FE038 (1.0)
extern f32 lbl_805FE03C;       // 0x805FE03C (0.0)
extern f32 lbl_805FE06C;       // 0x805FE06C (epsilon)
}
extern "C++" void fn_8015A2E8(u8* p, u32 unused4, u32 flush);  // 0x8015A2E8 (C++ linkage)
extern "C++" void* gEventMgr;     // 0x805FB6C8
extern "C++" void* gScreenSets;   // 0x805FB654

extern "C" {

// 0x8007D004
// Tallies the cells of a grid (two flag nibbles per byte, clearing the counted bits with 0xDD),
// then moves the meter at +0x14 by the weighted counts and clamps it to 0..100.
void fn_8007D004(u8* arg0, u8* arg1, s32 arg2, s32 arg3) {
    s32 sp8;
    if (arg3 == 0) return;

    u8* base = fn_8015A210(arg1, 0, &sp8, 0);
    s32 neg = 0;
    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    for (s32 row = 0; row < arg3; row++) {
        u8* p = base + sp8 * arg2 + row * sp8;
        for (s32 i = 0; i < (s32)(*(u16*)arg1 >> 1); i++) {
            u8 b = *p;
            u32 lo = b & 0xF;
            if (!((b >> 4) & 1)) {
                *(s32*)(arg0 + 0x4) += 1;
                if ((b >> 5) & 1) *(s32*)(arg0 + 0x8) += 1;
                else neg += 1;
            }
            if (!(lo & 1)) {
                *(s32*)(arg0 + 0x4) += 1;
                if ((lo >> 1) & 1) *(s32*)(arg0 + 0x8) += 1;
                else neg += 1;
            }
            *p = *p & 0xDD;
            p += 1;
        }
    }
    fn_8015A2E8(arg1, 0, 1);

    s32 total = *(s32*)(arg0 + 0x4);
    s32 good = *(s32*)(arg0 + 0x8);
    f32 mulGood = lbl_805FD760;
    f32 mulBad = mulGood;
    *(s32*)(arg0 + 0xC) += total;
    *(s32*)(arg0 + 0x10) += good;
    if (total > 0) {
        if (total == good) {
            mulGood = lbl_805FD764;
            fn_800712B8();
        } else if (good == 0) {
            mulBad = lbl_805FD768;
            fn_800712B4();
        }
    }
    f32 scale = *(f32*)(arg0 + 0x0);
    f32 v = (*(f32*)(arg0 + 0x14) + mulGood * (*(f32*)(arg0 + 0x18) * ((f32)good * scale)))
          - mulBad * (*(f32*)(arg0 + 0x1C) * ((f32)neg * scale));
    *(f32*)(arg0 + 0x14) = v;
    if (v > lbl_805FD750) {
        *(f32*)(arg0 + 0x14) = lbl_805FD750;
        return;
    }
    if (v < lbl_805FD748) {
        *(f32*)(arg0 + 0x14) = lbl_805FD748;
    }
}

// 0x8007D2C8
// Experiment screen: when the current screen set is UI_MSA_Experiments, lights the first N element
// widgets (N = 0..5 from the meter value) with the meter colour and hides the rest.
void fn_8007D2C8(u8* arg0) {
    u32 hashes[5];
    void* sets = ScreenSetsCurrent(gScreenSets);
    if (*(u32*)sets != 0xF611F736u) return;  // UI_MSA_Experiments

    f32 pct = *(f32*)(arg0 + 0x14) / lbl_805FD750;
    fn_80070FAC(fn_80078BB0(), pct);
    s32 n = (s32)(lbl_805FD76C * fn_8007C138(*(u8**)(fn_80078BB0() + 0x75C)));
    if (n > 5) n = 5;
    *(s32*)(arg0 + 0x20) = n;

    f32 r = lbl_805FD748;
    f32 g = r;
    f32 b = r;
    for (s32 i = 0; i < 5; i++) hashes[i] = lbl_80347D60[i];
    if (n < 6) {
        if (n >= 1) {
            r = lbl_803474C8[0];
            g = lbl_803474C8[1];
            b = lbl_803474C8[2];
        }
    }
    f32 a = lbl_805FD760;

    s32 i = 0;
    for (; i < n; i++) {
        u8* el = (u8*)ScreenSetFindElement(sets, hashes[i]);
        if (el != NULL) {
            (*(void (**)(u8*, s32))(*(u8**)(el + 0x138) + 0x2C))(el, 0x22);
            *(f32*)(el + 0x80) = r;
            *(f32*)(el + 0x84) = g;
            *(f32*)(el + 0x88) = b;
            *(f32*)(el + 0x8C) = a;
        }
    }
    for (; n < 5; n++) {
        u8* el = (u8*)ScreenSetFindElement(sets, hashes[n]);
        if (el != NULL) {
            (*(void (**)(u8*, s32))(*(u8**)(el + 0x138) + 0x30))(el, 2);
        }
    }
}

// 0x8008751C
// Finds the HUD child widgets: three progress bars (+0x7F0/+0x7F4/+0x7F8, each reset to 0..100
// with its own colour) and four plain children (+0x7FC..+0x808), then clears +0x7EC.
void fn_8008751C(u8* arg0) {
    u8* hud = (u8*)fn_80070F4C();
    if (hud != NULL) {
        u8* w = (u8*)UIWidgetFindChild(hud, 0x57EFBC28u);
        *(u8**)(arg0 + 0x7F0) = w;
        if (w != NULL) {
            fn_801A403C(w, lbl_805FD928);
            fn_801A4050(*(u8**)(arg0 + 0x7F0), lbl_805FD944);
            UISetProgress(*(u8**)(arg0 + 0x7F0), lbl_805FD928);
            u8* w2 = *(u8**)(arg0 + 0x7F0);
            *(f32*)(w2 + 0x80) = lbl_805FD928;
            *(f32*)(w2 + 0x84) = lbl_805FD948;
            *(f32*)(w2 + 0x88) = lbl_805FD934;
            *(f32*)(w2 + 0x8C) = lbl_805FD934;
            fn_801A4078(*(u8**)(arg0 + 0x7F0), lbl_805FD94C);
        }
        w = (u8*)UIWidgetFindChild(hud, 0xCEE6ED92u);
        *(u8**)(arg0 + 0x7F4) = w;
        if (w != NULL) {
            fn_801A403C(w, lbl_805FD928);
            fn_801A4050(*(u8**)(arg0 + 0x7F4), lbl_805FD944);
            UISetProgress(*(u8**)(arg0 + 0x7F4), lbl_805FD928);
            u8* w2 = *(u8**)(arg0 + 0x7F4);
            *(f32*)(w2 + 0x80) = lbl_805FD934;
            *(f32*)(w2 + 0x84) = lbl_805FD934;
            *(f32*)(w2 + 0x88) = lbl_805FD940;
            *(f32*)(w2 + 0x8C) = lbl_805FD934;
            fn_801A4078(*(u8**)(arg0 + 0x7F4), lbl_805FD94C);
        }
        w = (u8*)UIWidgetFindChild(hud, 0xB9E1DD04u);
        *(u8**)(arg0 + 0x7F8) = w;
        if (w != NULL) {
            fn_801A403C(w, lbl_805FD928);
            fn_801A4050(*(u8**)(arg0 + 0x7F8), lbl_805FD944);
            UISetProgress(*(u8**)(arg0 + 0x7F8), lbl_805FD928);
            u8* w2 = *(u8**)(arg0 + 0x7F8);
            *(f32*)(w2 + 0x80) = lbl_805FD950;
            *(f32*)(w2 + 0x84) = lbl_805FD934;
            *(f32*)(w2 + 0x88) = lbl_805FD950;
            *(f32*)(w2 + 0x8C) = lbl_805FD934;
            fn_801A4078(*(u8**)(arg0 + 0x7F8), lbl_805FD94C);
        }
        *(void**)(arg0 + 0x7FC) = UIWidgetFindChild(hud, 0x62119E68u);
        *(void**)(arg0 + 0x800) = UIWidgetFindChild(hud, 0xFB18CFD2u);
        *(void**)(arg0 + 0x804) = UIWidgetFindChild(hud, 0x8C1FFF44u);
        UIWidgetFindChild(hud, 0xA30F7B81u);  // result unused
        *(void**)(arg0 + 0x808) = UIWidgetFindChild(hud, 0x08A85674u);
    }
    *(s32*)(arg0 + 0x7EC) = 0;
}

// 0x8009AC44
// Spawn timer: counts down the timer at +0x94 (unless the value at [+0x198]+0x3EC is >= 0); when
// it expires, picks the first free slot of the array at +0x19C, rolls its kind against a table
// and, if a position along the path at +0x9C fits, places the slot (position + orientation).
void fn_8009AC44(u8* arg0, f32 dt) {
    f32 pos[3];
    f32 quat[4];

    if (!(*(f32*)(*(u8**)(arg0 + 0x198) + 0x3EC) >= lbl_805FDC50)) {
        *(f32*)(arg0 + 0x94) = *(f32*)(arg0 + 0x94) - dt;
    }
    if (!(*(f32*)(arg0 + 0x94) < lbl_805FDC50)) return;

    u8* slot = NULL;
    s32 i = 0;
    s32 off = 0;
    while (i < *(s32*)(arg0 + 0x1A0) && slot == NULL) {
        u8* cand = *(u8**)(*(u8**)(arg0 + 0x19C) + off);
        if (*(u8*)(cand + 0x270) == 0) {
            slot = cand;
            u32 rnd = RandomU32();
            f32 roll = lbl_805FDC7C * (f32)rnd;
            // the asm calls RaySceneTest with no argument (r3 is leftover)
            f32 limit = lbl_80346378[RaySceneTest(0)];
            fn_8009E2AC(cand, roll < limit ? 0 : 1);
        }
        off += 4;
        i += 1;
    }
    if (slot != NULL) {
        u8* path = arg0 + 0x9C;
        f32 len = fn_800C3E84(path);
        u8* tbl = *(u8**)(arg0 + 0x170);
        f32 step = lbl_805FDC80 * len;
        f32 t = (*(f32(**)(u8*))(tbl + 0x20))(path) + step;

        s32 count = *(s32*)(arg0 + 0xF8);
        if (count > 1) {
            u8* q = arg0 + 4;
            u8* cur = NULL;
            s32 k = 1;
            for (s32 left = count - 1; left != 0; left--) {
                if (k == 1) cur = *(u8**)(q + 0xA4);
                f32 a = *(f32*)(cur + 0x264);
                cur = *(u8**)(q + 0xA8);
                if (t < a) {
                    t = lbl_805FDC5C * (a + *(f32*)(cur + 0x264));
                    break;
                }
                q += 4;
                k += 1;
            }
        }
        if (t + step < len) {
            (*(void (**)(f32*, u8*, f32))(tbl + 0x24))(pos, path, t);
            *(f32*)(slot + 0x274) = pos[0];
            *(f32*)(slot + 0x278) = pos[1];
            *(f32*)(slot + 0x27C) = pos[2];
            fn_800C597C((u8*)quat, path, t);
            *(f32*)(slot + 0x280) = quat[0];
            *(f32*)(slot + 0x284) = quat[1];
            *(f32*)(slot + 0x288) = quat[2];
            *(f32*)(slot + 0x28C) = quat[3];
            fn_800C324C(path, slot, t);
        }
    }
    // the asm calls RaySceneTest with no argument (r3 is leftover)
    *(f32*)(arg0 + 0x94) = lbl_80346388[RaySceneTest(0)];
}

// 0x800A90EC
// Per-frame update of the stick-driven actor: ticks the timers at +0x6A0/+0x150, signals the
// game flow when the owner's value at vtable +0x64 runs out, applies stick input to +0x71C,
// sends contact events for the touched list, and refreshes the HUD widget at +0x6A4.
void fn_800A90EC(u8* arg0, f32 dt) {
    f32 stickX = lbl_805FDF18;
    f32 stickY = lbl_805FDF18;
    void* list[5] = {0, 0, 0, 0, 0};
    u32 var_r31 = 1;

    f32 t = *(f32*)(arg0 + 0x6A0) - dt;
    *(f32*)(arg0 + 0x6A0) = t;
    *(f32*)(arg0 + 0x150) = *(f32*)(arg0 + 0x150) - dt;
    if (t <= lbl_805FDF18) {
        f32 v = (*(f32(**)(u8*))(*(u8**)(arg0 + 0x4) + 0x64))(arg0);
        if (v > lbl_805FDF18) GameFlowSignal((s32)0xC608C842u, 0, 0);
        else GameFlowSignal((s32)0x117741C3u, 0, 0);
        var_r31 = 0;
    }
    if ((*(f32(**)(u8*))(*(u8**)(arg0 + 0x4) + 0x64))(arg0) <= lbl_805FDF18) {
        f32 v = (*(f32(**)(u8*))(*(u8**)(arg0 + 0x4) + 0x64))(arg0);
        if (v > lbl_805FDF18) GameFlowSignal((s32)0xC608C842u, 0, 0);
        else GameFlowSignal((s32)0x117741C3u, 0, 0);
        var_r31 = 0;
    }

    f32 oldVal = *(f32*)(arg0 + 0x71C);
    StickRead(*(s32*)(lbl_803C4298 + 0x38) * 16 + 0xE, &stickX, &stickY);
    if (fabsf(stickX) >= lbl_805FDF58) {
        *(f32*)(arg0 + 0x71C) = *(f32*)(arg0 + 0x71C) + dt * (lbl_805FDF54 * stickX);
    }
    f32 newVal = *(f32*)(arg0 + 0x71C) + dt * *(f32*)(arg0 + 0x724);
    *(f32*)(arg0 + 0x71C) = newVal;
    fn_800AA2D0(arg0, dt, newVal);
    fn_800A9B9C(arg0, dt);
    fn_800AA094(arg0, dt);
    fn_800AA2D0(arg0, dt, oldVal);

    s32 count = fn_800B0530(*(u8**)(arg0 + 0xB0));
    if (count > 0 && fn_800B058C(*(u8**)(arg0 + 0xB0), list, 5) != 0) {
        for (s32 i = 0; i < count; i++) {
            u8* item = (u8*)list[i];
            u8* obj = fn_800ABD80(item);
            u8* src = *(u8**)(item + 0x4);
            *(f32*)(obj + 0x14) = *(f32*)(src + 0x0);
            *(f32*)(obj + 0x18) = *(f32*)(src + 0x4);
            *(f32*)(obj + 0x1C) = *(f32*)(src + 0x8);
            item = (u8*)list[i];
            *(f32*)(obj + 0x20) = *(f32*)(item + 0x27C);
            *(f32*)(obj + 0x24) = *(f32*)(item + 0x280);
            *(f32*)(obj + 0x28) = *(f32*)(item + 0x284);
            // asm passes only (self, obj); r5 and f1/f2 are leftovers
            u32 ev = fn_800AA9D4(arg0, obj);
            if (ev != 0xFFFFFFFFu) {
                u8* target = (u8*)list[i];
                u8* data = *(u8**)(target + 0x4);
                s32 flags = *(s32*)(data + 0x20);
                if (!(flags & 1)) {
                    *(s32*)(data + 0x20) = flags | 1;
                    EventSend(gEventMgr, target, NULL, ev, 0, 0, 1, 0);
                    u8* data2 = *(u8**)(target + 0x4);
                    *(s32*)(data2 + 0x20) = *(s32*)(data2 + 0x20) & ~1;
                } else {
                    EventSend(gEventMgr, target, NULL, ev, 0, 0, 1, 0);
                }
            }
        }
    }

    f32 timer = *(f32*)(arg0 + 0x6A0);
    fn_800712BC(arg0, (s32)lrintf(timer), timer < lbl_805FDF48);  // fctiw: round to nearest
    fn_80070FAC(arg0, (*(f32(**)(u8*))(*(u8**)(arg0 + 0x4) + 0x64))(arg0) / lbl_805FDF5C);
    s32 flag = fn_800B13B8(*(u8**)(arg0 + 0xB4));
    fn_800715B8(arg0, flag);
    fn_80071838(arg0, flag == 0);
    if (flag == 0 && var_r31 == 1) {
        u8* w = *(u8**)(arg0 + 0x6A4);
        if (*(s32*)(w + 0x10) == 0) {
            UIWidgetPlayAnimLoop(w, 0x607200BEu, 0xA);
        }
        w = *(u8**)(arg0 + 0x6A4);
        (*(void (**)(u8*, s32))(*(u8**)(w + 0x138) + 0x2C))(w, 2);
        return;
    }
    u8* w = *(u8**)(arg0 + 0x6A4);
    if (*(s32*)(w + 0x10) != 0) {
        fn_8019BC80(w, 1);
    }
    w = *(u8**)(arg0 + 0x6A4);
    (*(void (**)(u8*, s32))(*(u8**)(w + 0x138) + 0x30))(w, 2);
}

// 0x800AEFEC
// Sweep test of the moving sphere arg1 (position +0x14, previous position +0x20, radius +0x3C)
// against the array of obstacles at arg0 (+0xB4 entries; each is a sphere, or a capsule when
// the second array at +0x3C holds an end point). On the first hit with entry time < 1 it returns
// fn_800AF3E0(arg0, &pos, &dir); returns -1 when nothing is hit. arg2 (optional) receives the
// contact point.
s32 fn_800AEFEC(u8* arg0, u8* arg1, f32* arg2) {
    f32 dir[3];   // sp7C
    f32 t0 = 0.0f;
    f32 t1 = 0.0f;
    f32 seg[3];
    u8* entry = arg0;

    dir[0] = *(f32*)(arg1 + 0x14) - *(f32*)(arg1 + 0x20);
    dir[1] = *(f32*)(arg1 + 0x18) - *(f32*)(arg1 + 0x24);
    dir[2] = *(f32*)(arg1 + 0x1C) - *(f32*)(arg1 + 0x28);

    for (s32 i = 0; i < *(s32*)(arg0 + 0xB4); i++, entry += 4) {
        s32 hit;
        f32 t = lbl_805FE03C;
        u8* end = *(u8**)(entry + 0x3C);
        if (end != NULL) {
            // capsule: from the entry's first point to its end point
            t0 = lbl_805FE03C;
            t1 = lbl_805FE03C;
            u8* a = *(u8**)(entry + 0x0);
            f32 rEnd = *(f32*)(end + 0x3C);
            f32 rA = *(f32*)(a + 0x3C);
            f32 rMe = *(f32*)(arg1 + 0x3C);
            if (!(rA < rEnd)) rA = rEnd;
            seg[0] = *(f32*)(end + 0x14) - *(f32*)(a + 0x14);
            seg[1] = *(f32*)(end + 0x18) - *(f32*)(a + 0x18);
            seg[2] = *(f32*)(end + 0x1C) - *(f32*)(a + 0x1C);
            f32 d2 = fn_801BFE38(a + 0x14, seg, arg1 + 0x20, (u8*)dir, &t0, &t1);
            f32 rr = rMe + rA;
            hit = (d2 <= rr * rr);
            t = lbl_805FE03C;
            if (hit && arg2 != 0) {
                u8* p0 = *(u8**)(entry + 0x0);
                u8* p1 = *(u8**)(entry + 0x3C);
                f32 dx = *(f32*)(p1 + 0x14) - *(f32*)(p0 + 0x14);
                f32 dy = *(f32*)(p1 + 0x18) - *(f32*)(p0 + 0x18);
                f32 dz = *(f32*)(p1 + 0x1C) - *(f32*)(p0 + 0x1C);
                arg2[0] = *(f32*)(p0 + 0x14) + dx * t0;
                arg2[1] = *(f32*)(p0 + 0x18) + dy * t0;
                arg2[2] = *(f32*)(p0 + 0x1C) + dz * t0;
            }
        } else {
            // sphere: ray (arg1 start, -dir relative) against the entry's sphere
            u8* c = *(u8**)(entry + 0x0);
            f32 vx = lbl_805FE03C - dir[0];
            f32 dx = *(f32*)(c + 0x14) - *(f32*)(arg1 + 0x20);
            f32 vy = lbl_805FE03C - dir[1];
            f32 dy = *(f32*)(c + 0x18) - *(f32*)(arg1 + 0x24);
            f32 vz = lbl_805FE03C - dir[2];
            f32 dz = *(f32*)(c + 0x1C) - *(f32*)(arg1 + 0x28);
            f32 rsum = *(f32*)(c + 0x3C) + *(f32*)(arg1 + 0x3C);
            f32 cc = (dz * dz + (dx * dx + dy * dy)) - rsum * rsum;
            if (cc < 0.0f) {
                hit = 1;
            } else {
                f32 a = vz * vz + (vx * vx + vy * vy);
                if (a < lbl_805FE06C) {
                    hit = 0;
                } else {
                    f32 b = vz * dz + (vx * dx + vy * dy);
                    if (b >= 0.0f) {
                        hit = 0;
                    } else {
                        f32 disc = b * b - a * cc;
                        if (disc < 0.0f) {
                            hit = 0;
                        } else {
                            f32 s = 0.0f;
                            if (!(disc <= 0.0f)) {
                                s = sqrtf(disc);  // asm: frsqrte + two Newton steps
                            }
                            hit = 1;
                            t = (-b - s) / a;
                        }
                    }
                }
            }
            if (hit && arg2 != 0) {
                arg2[0] = *(f32*)(c + 0x14);
                arg2[1] = *(f32*)(c + 0x18);
                arg2[2] = *(f32*)(c + 0x1C);
            }
        }
        if (hit == 1 && t < lbl_805FE038) {
            f32 sweepDir[3] = { dir[0], dir[1], dir[2] };
            f32 sweepPos[3] = { *(f32*)(arg1 + 0x14), *(f32*)(arg1 + 0x18), *(f32*)(arg1 + 0x1C) };
            // asm passes (self, &pos, &dir); f1/f2/f0 are leftovers
            return fn_800AF3E0(arg0, (u8*)sweepPos, (u8*)sweepDir);
        }
    }
    return -1;
}

}  // extern "C"
