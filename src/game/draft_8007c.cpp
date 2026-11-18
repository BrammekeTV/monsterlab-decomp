// Experiment HUD widgets and small helpers in 0x8007xxxx.
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"

void  CameraSetMatrix(void* camera, const f32 mtx[3][4]);          // signature from draft
void  LabShowBattleResult(void* lab, const void* result);           // 0x802186D8
s32   PlaySoundAt(void* sound, int a, const void* pos, int b);      // 0x80209ABC
void  SaveRewardExperiment(u8* s, u32 experiment, BOOL a);
void* ScreenSetFindWidget(void* set, u32 hash);                     // 0x80198E34
void* ScreenSetsCurrent(void* sets);                                // 0x8018D1E4
u32   StringHash(const char* s);                                    // 0x801EB704
void  UIPlayAnim(void* el, u32 anim);                               // 0x8019BE40
void  UISetNumber(void* el, s32 n);                                 // 0x8019F2A8
void  UISetText(void* el, const char* text, s32 a);                 // 0x8019EFC8
void* UIWidgetFindChild(void* widget, u32 hash);
void  fn_80009840(u8* p);                                           // 0x80009840
void* fn_800701A4(void);                                            // 0x800701A4 (jump table; per-lab-state HUD config or NULL)
void  fn_8016F854(f32* out, void* a, s32 b);                        // 0x8016F854 (signature guessed)
void* fn_801D40E0(s32 n);                                           // 0x801D40E0
void* fn_80209814(void* v);                                         // 0x80209814
void  fn_8020A398(s32 handle);                                      // 0x8020A398

extern void* gLab;                // 0x805FBBC8
extern s32 gScreenHeight;         // 0x805F938C
extern void* gScreenSets;         // 0x805FB654
extern s32 gScreenWidth;          // 0x805F9388
extern u8 gWorldState[];          // 0x803E6D00 (+0xC frame delta)
extern u8 lbl_803C4298[];         // 0x803C4298 (+4: camera object with vtable)
extern u8 lbl_803C5740[];         // 0x803C5740 (experiment sound table, 0x10 bytes per entry)
extern u8 lbl_80345FCC[];         // 0x80345FCC (3 floats: bar colour)
extern u8 lbl_80345FD8[];         // 0x80345FD8
extern u8 lbl_80345FE4[];         // 0x80345FE4
extern u8 lbl_80345FF0[];         // 0x80345FF0
extern u8 lbl_8047C640[];         // 0x8047C640
extern s32 lbl_805F88C8;          // 0x805F88C8 (last shown timer seconds)
extern const char lbl_805F88CC[]; // 0x805F88CC (":")
extern s32 lbl_805FB12C;          // 0x805FB12C (sound table entry count)
extern u8* lbl_805FB134;          // 0x805FB134 (object, tag at +0 == 1)
extern void* lbl_805FB5F8;        // 0x805FB5F8
extern void* lbl_805FBBAC;        // 0x805FBBAC (voice SFX handle)
extern u8* lbl_805FBBE0;          // 0x805FBBE0 (save block pointer)
extern f32 lbl_805FD5DC;          // 0x805FD5DC
extern f32 lbl_805FD5FC;          // 0x805FD5FC
extern f32 lbl_805FD600;          // 0x805FD600 (bar width scale)
extern f32 lbl_805FD604;          // 0x805FD604
extern f32 lbl_805FD608;          // 0x805FD608
extern f32 lbl_805FD60C;          // 0x805FD60C
extern f32 lbl_805FD610;          // 0x805FD610
extern f32 lbl_805FD614;          // 0x805FD614

typedef void (*WFn)(void* self, s32 a);
typedef f32 (*WFnF)(void* self);

// Looks up the HUD screen widget with the given name hash if the current lab state's HUD
// config has `flag` set; NULL otherwise.
static u8* HudScreen(u32 flag, u32 hash) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return NULL;
    u8* cfg = (u8*)fn_800701A4();
    if (cfg == NULL) return NULL;
    if (*(u32*)(cfg + 0x10) & flag) return (u8*)ScreenSetFindWidget(set, hash);
    return NULL;
}

// Widget vtable (pointer at +0x138): slot +0x2C show, +0x30 hide, both (widget, mode).
static inline void WidgetShow(u8* w, s32 a) { (*(WFn*)(*(u8**)(w + 0x138) + 0x2C))(w, a); }
static inline void WidgetHide(u8* w, s32 a) { (*(WFn*)(*(u8**)(w + 0x138) + 0x30))(w, a); }

// 0x80070FD8
// Updates the three bar widgets from the value at +0x84: the first is placed directly, the
// other two ease toward it (rise / fall per frame delta); the second is recoloured in four steps.
void fn_80070FD8(u8* arg0) {
    u8* screen = HudScreen(0x1, 0x634B9C91);
    if (screen == NULL) return;

    u8* w = (u8*)UIWidgetFindChild(screen, 0x1183227A);
    if (w != NULL) {
        *(f32*)(w + 0x68) = lbl_805FD5FC + lbl_805FD600 * *(f32*)(arg0 + 0x84);
        WidgetShow(w, 0x18);
    }

    w = (u8*)UIWidgetFindChild(screen, 0xE8903BD7);
    if (w != NULL) {
        f32 target = *(f32*)(arg0 + 0x84);
        f32 cur = *(f32*)(arg0 + 0x7C);
        if (target > cur) {
            *(f32*)(arg0 + 0x7C) = cur + lbl_805FD604 * *(f32*)(gWorldState + 0xC);
        } else {
            *(f32*)(arg0 + 0x7C) = target;
        }
        *(f32*)(w + 0x74) = lbl_805FD600 * *(f32*)(arg0 + 0x7C);
        f32 v = *(f32*)(arg0 + 0x84);
        const f32* col;
        if (v < lbl_805FD608) {
            col = (const f32*)lbl_80345FCC;
        } else if (v < lbl_805FD5DC) {
            col = (const f32*)lbl_80345FD8;
        } else if (v < lbl_805FD610) {
            col = (const f32*)lbl_80345FE4;
        } else {
            col = (const f32*)lbl_80345FF0;
        }
        *(f32*)(w + 0x80) = col[0];
        *(f32*)(w + 0x84) = col[1];
        *(f32*)(w + 0x88) = col[2];
        *(f32*)(w + 0x8C) = lbl_805FD60C;
        WidgetShow(w, 0x38);
    }

    w = (u8*)UIWidgetFindChild(screen, 0xCB5F5088);
    if (w != NULL) {
        f32 target = *(f32*)(arg0 + 0x84);
        f32 cur = *(f32*)(arg0 + 0x80);
        if (target < cur) {
            f32 n = cur + lbl_805FD614 * *(f32*)(gWorldState + 0xC);
            *(f32*)(arg0 + 0x80) = n;
            if (n < target) *(f32*)(arg0 + 0x80) = target;
        } else {
            *(f32*)(arg0 + 0x80) = target;
        }
        *(f32*)(w + 0x74) = lbl_805FD600 * *(f32*)(arg0 + 0x80);
        WidgetShow(w, 0x38);
    }
}

// 0x800712BC
// Countdown timer HUD: shows |arg1| as M:SS (digit widgets) when it changed; with arg2 != 0
// starts the countdown anim and sound, otherwise hides the timer widget.
void fn_800712BC(u8* arg0, s32 arg1, s32 arg2) {
    u8* screen = HudScreen(0x2, 0x4A0420DE);
    if (screen == NULL) return;

    s32 secs = arg1 < 0 ? -arg1 : arg1;
    u8* d[4];
    d[0] = (u8*)UIWidgetFindChild(screen, 0xE3D91B28);
    d[1] = (u8*)UIWidgetFindChild(screen, 0x7AD04A92);
    d[2] = (u8*)UIWidgetFindChild(screen, 0x0DD77A04);
    d[3] = (u8*)UIWidgetFindChild(screen, 0x93B3EFA7);
    if (lbl_805F88C8 != secs) {
        s32 s = secs % 60;
        UISetNumber(d[0], s % 10);
        UISetNumber(d[1], s / 10);
        UISetText(d[2], lbl_805F88CC, 1);
        UISetNumber(d[3], secs / 60);
        lbl_805F88C8 = secs;
    }
    if (arg2 != 0) {
        u8* w = (u8*)UIWidgetFindChild(screen, 0x88847792);
        if (w != NULL && *(s32*)(w + 0x10) == 0) {
            UIPlayAnim(w, 0xCB4C4D10);
            WidgetShow(w, 2);
            u32 hash = StringHash("Generic_Timer_Countdown_Mo_01");
            u8* e = lbl_803C5740;
            for (s32 i = 0; i < lbl_805FB12C; i++, e += 0x10) {
                if (*(s32*)(e + 0x4) == 0 && *(u32*)(e + 0x8) == hash) {
                    s32 h = *(s32*)(e + 0xC);
                    if (h != -1) fn_8020A398(h);
                    u8* snd = *(u8**)(e + 0x0);
                    if (snd != NULL) *(u8*)(snd + 0x2E) = 0;
                    if (lbl_805FBBAC == NULL) {
                        void* v = fn_801D40E0(1);
                        if (v != NULL) v = fn_80209814(v);
                        lbl_805FBBAC = v;
                    }
                    *(s32*)(e + 0xC) = PlaySoundAt(*(void**)(e + 0x0), 0, NULL, 0);
                }
            }
        }
        for (s32 i = 0; i < 4; i++) {
            if (*(s32*)(d[i] + 0x10) == 0) UIPlayAnim(d[i], 0xDB3BF285);
        }
    } else {
        u8* w = (u8*)UIWidgetFindChild(screen, 0x88847792);
        if (w != NULL) WidgetHide(w, 2);
    }
}

// 0x800715B8
// Two-digit counter HUD: sets the ones and tens digit widgets from |arg1|.
void fn_800715B8(u8* arg0, s32 arg1) {
    u8* screen = HudScreen(0x4000, 0x4AD84AA2);
    if (screen == NULL) return;
    s32 n = arg1 < 0 ? -arg1 : arg1;
    void* ones = UIWidgetFindChild(screen, 0x4DD2D818);
    void* tens = UIWidgetFindChild(screen, 0xD4DB89A2);
    UISetNumber(ones, n % 10);
    UISetNumber(tens, n / 10);
}

// 0x800716D0
// arg1 == 1 shows widget 0xFBB7DDF8 and hides 0xAEE238D4, otherwise the reverse; then hides
// 0xF0414F47 and 0xA514AA6B.
void fn_800716D0(u8* arg0, u32 arg1) {
    u8* screen = HudScreen(0x4000, 0x4AD84AA2);
    if (screen == NULL) return;
    u8* a = (u8*)UIWidgetFindChild(screen, 0xAEE238D4);
    u8* b = (u8*)UIWidgetFindChild(screen, 0xFBB7DDF8);
    if (arg1 == 1) {
        WidgetShow(b, 2);
        WidgetHide(a, 2);
    } else {
        WidgetHide(b, 2);
        WidgetShow(a, 2);
    }
    WidgetHide((u8*)UIWidgetFindChild(screen, 0xF0414F47), 2);
    WidgetHide((u8*)UIWidgetFindChild(screen, 0xA514AA6B), 2);
}

// 0x80071838
// arg1 == 1 shows widget 0xA514AA6B and hides 0xF0414F47, otherwise the reverse; then hides
// 0xAEE238D4 and 0xFBB7DDF8.
void fn_80071838(u8* arg0, u32 arg1) {
    u8* screen = HudScreen(0x4000, 0x4AD84AA2);
    if (screen == NULL) return;
    u8* a = (u8*)UIWidgetFindChild(screen, 0xF0414F47);
    u8* b = (u8*)UIWidgetFindChild(screen, 0xA514AA6B);
    if (arg1 == 1) {
        WidgetShow(b, 2);
        WidgetHide(a, 2);
    } else {
        WidgetHide(b, 2);
        WidgetShow(a, 2);
    }
    WidgetHide((u8*)UIWidgetFindChild(screen, 0xAEE238D4), 2);
    WidgetHide((u8*)UIWidgetFindChild(screen, 0xFBB7DDF8), 2);
}

// 0x800719A0
// Builds the battle result record (flag, score, ratio of the +0x74/+0x78 floats) from the
// tag-1 object in lbl_805FB134 and shows it; on success saves the reward experiment.
void SetFinalExperimentScore(s32 a0) {
    struct { u8 ok; s32 score; s32 ratio; s32 pad; s32 zero; } r;
    u8* o = lbl_805FB134;
    if (o != NULL && *(s32*)o != 1) o = NULL;
    f32 score = (*(WFnF*)(*(u8**)(o + 0x4) + 0x64))(o);
    o = lbl_805FB134;
    if (o != NULL && *(s32*)o != 1) o = NULL;
    f32 ratio = *(f32*)(o + 0x74) / *(f32*)(o + 0x78);
    if (a0 != 0) {
        r.ok = 1;
        r.score = (s32)score;
        r.ratio = (s32)ratio;
    } else {
        r.ok = 0;
        r.score = 0;
        r.ratio = 0;
    }
    r.zero = 0;
    LabShowBattleResult(gLab, &r);
    if (r.ok != 0) {
        SaveRewardExperiment(lbl_805FBBE0, *(u32*)((u8*)gLab + 0x48), r.score);
    }
}

// 0x80073A7C
// Converts a normalised screen position to pixels: *arg2 = width * x, *arg3 = height * y.
// With arg0 == NULL it first refreshes the camera matrix from the camera object (vtable slot 0x38).
void fn_80073A7C(void* arg0, s32 arg1, f32* arg2, f32* arg3) {
    f32 pos[2];
    f32 mtx[3][4];
    if (arg0 == NULL) {
        u8* cam = *(u8**)(lbl_803C4298 + 4);
        typedef void (*Fn)(void* self, void* out);
        (*(Fn*)(*(u8**)cam + 0x38))(cam, mtx);
        CameraSetMatrix(lbl_8047C640, mtx);
        fn_80009840(lbl_803C4298);
        arg0 = lbl_805FB5F8;
    }
    fn_8016F854(pos, arg0, arg1);
    *arg2 = (f32)gScreenWidth * pos[0];
    *arg3 = (f32)gScreenHeight * pos[1];
}

}  // extern "C"
