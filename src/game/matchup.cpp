// Online matchup screen (script callback MatchupUpdate): the two monster previews, the
// waiting / ready panels, the countdown, and, once the connect bar fills, building the
// opponent's monster from the received part list and starting the fight.
// Original: MatchupUpdate 0x8013CFA8 (auto_03_8013CF8C_text.s)
#include <stddef.h>

#include "types.h"
#include "game/part.h"
#include "game/script_callbacks.h"

extern "C" {
BOOL   MatchupIsActive(void);                              // 0x801409FC
void*  fn_801B7198(void* anim, s32 a);
s32    fn_8004E300(void* a, s32 b);
void*  MatchupGetRemoteData(void);                         // 0x80113974
void   fn_801A3010(void* obj, void* data);
void   fn_8013E0EC(void);  void fn_8013DE7C(void);
void   MatchupShowStatus(BOOL waiting);                    // 0x8013DC54
void   MatchupSetReady(BOOL ready);                        // 0x8013F2A4
void   MatchupSetHighlight(void* panel, u32 hash, BOOL on);   // 0x8013FBCC
void*  ScreenSetsCurrent(void* sets);                      // 0x8018D1E4
void*  ScreenSetFindWidget(void* set, u32 hash);           // 0x80198E34
void*  UIWidgetFindChild(void* w, u32 hash);
void   UIWidgetPlayAnim(void* w, u32 anim);                // 0x80199B38
void   UISetText(void* el, const char* text, s32 a);       // 0x8019EFC8
void   UISetProgress(void* widget, f32 t);                 // 0x801A3FA0
void   fn_801F5724(u32 button);
s32   SysSprintf(char* buf, const char* fmt, ...);
void*  EventPost(s32 id, void* target);                    // 0x801F3918
void   PartInit(PartInstance* p);                          // 0x8021D320
void   PartDestroy(PartInstance* p, s32 del);              // 0x8021D374
void   PartClear(PartInstance* p);                         // 0x8021D408
void   PartSetDef(PartInstance* p, s32 def);               // 0x8021D43C
void   PartSetQuality(PartInstance* p, s32 q);             // 0x8021DA40
void   PartSetTier(PartInstance* p, u8 tier);              // 0x8021DA68
void   PartSetDefect(PartInstance* p, s32 d);              // 0x8021DA80
void   PartSetEnhancement(PartInstance* p, s32 e);         // 0x8021DA88
void   MonsterSetName(void* monster, const char* name);    // 0x8021C8E0
void   MonsterCopyName(void* dst, void* src);              // 0x8020EBB8
}

extern void*  gScreenSets;          // 0x805FB654
extern u8     gGame[];              // 0x803B53A0
extern u8     gPlayerNames[];       // 0x80556E80 (+0x340: 32-byte names)
extern u32    gAlertPlayerWord;     ORIG(0x805F9030)
extern f32    gFrameTime;           ORIG(0x805FB918)
extern char   gLocalMonsterName[];  ORIG(0x803F2088)

// Our own monster as stored on the save (5 limbs).
struct SavedPart { s32 def; u8 tier; s8 quality; u8 pad[2]; s32 enhancement; s32 defect; };
extern SavedPart gLocalParts[5];    ORIG(0x803E69D8)

static u8*   sLocalPreview;         ORIG(0x805FB3BC)
static u8*   sRemotePreview;        ORIG(0x805FB3C8)
static u8*   sRemoteData;           ORIG(0x805FB3C0)
static s32   sLocalDelay;           ORIG(0x805FB3E8)
static s32   sRemoteDelay;          ORIG(0x805FB3E4)
static u8    sRemoteReady;          ORIG(0x805FB3B5)
static u8    sOpponentWaiting;      ORIG(0x805FB3B8)   // non-zero once the opponent chose
static u8    sLocalConfirmed;       ORIG(0x805FB3B3)
static u8    sWaitPanelUp;          ORIG(0x805FB3B1)
static u8    sReadyPanelUp;         ORIG(0x805FB3B0)
static u8    sConnecting;           ORIG(0x805FB3B2)
static u8    sNamesShown;           ORIG(0x805FB3DE)
static u8    sSearchShown = 1;      ORIG(0x805F9081)
static u8    sLocalSpinner = 1;     ORIG(0x805F9080)
static u8    sCountdownOn;          ORIG(0x805FB414)
static f32   sCountdown;            ORIG(0x805FB410)
static f32   sConnectTime;          ORIG(0x805FB3D8)
static f32   sLocalPos[3]  = { -1.0f, -1.0f, -1.0f };   ORIG(0x805F9084)
static f32   sRemotePos[3] = { -1.0f, -1.0f, -1.0f };   ORIG(0x805F9090)

static const f32 kCrouchOffset[3] = { 0.0f, 0.0f, -0.2f };   // 0x80345FFC
#define CONNECT_SECONDS 5.0f

#define W_PANEL         0x9631294E
#define W_PANEL_ROOT    0x4DD41D3D
#define W_WAIT          0x01AAFF52
#define W_READY         0xA3556852
#define W_HIGHLIGHT     0xCC13FD8B
#define W_STATUS_A      0xE24CF5B1
#define W_STATUS_B      0x00682601
#define W_NAMES         0x5AFA58C8
#define W_SEARCHING     0xAF32EE45

typedef void (*VisFn)(void*, s32);
static inline void Show(u8* obj, s32 mode) { (*(VisFn*)(*(u8**)(obj + 0x138) + 0x2C))(obj, mode); }

// Keep a preview monster at its spot; lower it while it plays the crouch animation (13).
static void PlacePreview(u8* obj, f32 saved[3]) {
    f32* pos = (f32*)(obj + 0x68);
    if (saved[0] == -1.0f && saved[1] == -1.0f && saved[2] == -1.0f) {
        saved[0] = pos[0]; saved[1] = pos[1]; saved[2] = pos[2];
    }
    f32 p[3] = { saved[0], saved[1], saved[2] };
    void* anim = fn_801B7198(obj + 0x140, 0);
    if (anim != NULL && fn_8004E300(anim, 1) == 0xD) {
        p[0] += kCrouchOffset[0]; p[1] += kCrouchOffset[1]; p[2] += kCrouchOffset[2];
    }
    pos[0] = p[0]; pos[1] = p[1]; pos[2] = p[2];
    Show(obj, 0x18);
}

static void* Panel(void) {
    void* set = ScreenSetsCurrent(gScreenSets);
    return set != NULL ? ScreenSetFindWidget(set, W_PANEL) : NULL;
}

// Play an animation on one of the panel's sub-panels if it exists.
static BOOL PanelAnim(u32 child, u32 anim) {
    void* p = Panel();
    if (p == NULL || UIWidgetFindChild(p, W_PANEL_ROOT) == NULL || UIWidgetFindChild(p, child) == NULL) return FALSE;
    UIWidgetPlayAnim(p, anim);
    return TRUE;
}

static void SetStatus(BOOL visible) {
    UIElement_SetVisibleFlag(W_STATUS_A, visible);
    UIElement_SetVisibleFlag(W_STATUS_B, visible);
    MatchupShowStatus(!visible);
}

static void HideWaitPanel(void) {
    if (sWaitPanelUp && PanelAnim(W_WAIT, 0x4A36C76E)) sWaitPanelUp = 0;
}
static void HideReadyPanel(void) {
    if (sReadyPanelUp && PanelAnim(W_READY, 0xA51E8C39)) sReadyPanelUp = 0;
}
static void ClearHighlight(void) {
    if (void* p = Panel()) {
        UIElement_Reset(W_HIGHLIGHT);
        MatchupSetHighlight(p, W_HIGHLIGHT, FALSE);
    }
}

static void BuildMonsters(void) {
    EventPost(0x4D, NULL);
    PartInstance tmp;
    PartInit(&tmp);
    *(s32*)(gGame + 0xD62C) = 0;
    u8* remote = gGame + 0xAB98;
    MonsterCopyName(remote, sRemoteData + 0x898);
    for (s32 i = 0; i < 5; i++) {
        PartClear(&tmp);
        PartInstance* p = ((PartInstance**)(sRemoteData + 0x89C))[i];
        if (p != NULL && p->defIndex != -1) {
            PartSetDef(&tmp, p->defIndex);
            PartSetTier(&tmp, p->tier);
            PartSetQuality(&tmp, p->quality);
            PartSetEnhancement(&tmp, p->enhancement);
            PartSetDefect(&tmp, p->defect);
        }
        (*(void (**)(void*, s32, PartInstance*))(*(u8**)remote + 0x14))(remote, i, &tmp);
    }

    *(s32*)(gGame + 0xD630) = 1;
    u8* local = gGame + 0xACD4;
    for (s32 i = 0; i < 5; i++) {
        const SavedPart& s = gLocalParts[i];
        PartSetDef(&tmp, s.def);
        PartSetTier(&tmp, s.tier);
        PartSetQuality(&tmp, s.quality);
        PartSetEnhancement(&tmp, s.enhancement);
        PartSetDefect(&tmp, s.defect);
        (*(void (**)(void*, s32, PartInstance*))(*(u8**)local + 0x14))(local, i, &tmp);
    }
    MonsterSetName(local, gLocalMonsterName);

    UIElement_SetVisibleFlag(W_STATUS_A, 0);
    UIElement_SetVisibleFlag(W_STATUS_B, 0);
    MatchupShowStatus(FALSE);
    GameFlowSignal((s32)0xF7E43ED2, 0, 0);
    PartDestroy(&tmp, -1);
}

// 0x8013CFA8
void MatchupUpdate(void) {
    if (!MatchupIsActive()) return;

    if (sLocalPreview != NULL) PlacePreview(sLocalPreview, sLocalPos);
    if (sRemotePreview != NULL) PlacePreview(sRemotePreview, sRemotePos);

    if (sLocalPreview != NULL) {
        if (Creatures_GetCreatureLoaded() && !(*(u32*)(sLocalPreview + 0x64) & 2) && sLocalDelay == 0) {
            sRemoteData = (u8*)MatchupGetRemoteData();
            if (sRemoteData != NULL) {
                fn_801A3010(sLocalPreview, sRemoteData);
                sLocalDelay = 3;
            }
        }
        if (sLocalDelay > 0 && --sLocalDelay == 0) {
            if (sLocalSpinner) {
                sLocalSpinner = 0;
                UIElement_SetVisibleFlag(0x363BBFFF, 0);
            }
            Show(sLocalPreview, 2);
        }
    }

    fn_8013E0EC();
    fn_8013DE7C();

    if (sRemotePreview != NULL) {
        if (!(*(u32*)(sRemotePreview + 0x64) & 2) && sRemoteReady && sRemoteDelay == 0) sRemoteDelay = 3;
        if (sRemoteDelay > 0 && --sRemoteDelay == 0) Show(sRemotePreview, 2);
    }

    if (sRemoteReady && sRemoteDelay <= 0) {
        if (sSearchShown) {
            sSearchShown = 0;
            UIElement_SetVisibleFlag(W_SEARCHING, 0);
        }
        if (sNamesShown != 1) {
            sNamesShown = 1;
            UIElement_SetVisibleFlag(W_NAMES, 1);
        }
        if (!sOpponentWaiting) {
            SetStatus(TRUE);
            if (sWaitPanelUp != 1 && PanelAnim(W_WAIT, 0x2F283883)) sWaitPanelUp = 1;
            if (void* p = Panel()) MatchupSetHighlight(p, W_HIGHLIGHT, TRUE);
            MatchupSetReady(sLocalConfirmed == 1);
        } else {
            HideWaitPanel();
            ClearHighlight();
            if (!sLocalConfirmed) {
                SetStatus(TRUE);
                if (sReadyPanelUp != 1 && PanelAnim(W_READY, 0x63CC8B0B)) sReadyPanelUp = 1;
            } else {
                MatchupSetReady(FALSE);
                HideReadyPanel();
                SetStatus(FALSE);
            }
        }
    } else {
        ClearHighlight();
        UIElement_SetVisibleFlag(0x6530D136, 0);
        if (sNamesShown) {
            sNamesShown = 0;
            UIElement_SetVisibleFlag(W_NAMES, 0);
        }
        if (sSearchShown != 1) {
            sSearchShown = 1;
            UIElement_SetVisibleFlag(W_SEARCHING, 1);
        }
        HideWaitPanel();
        HideReadyPanel();
        SetStatus(TRUE);
        if (!sLocalConfirmed) MatchupSetReady(FALSE);
    }

    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;

    // Countdown "0:SS"; at zero, press A for the player.
    if (sCountdownOn == 1) {
        void* w = ScreenSetFindWidget(set, 0x43538242);
        if (w != NULL && UIWidgetFindChild(w, 0x9D841CAA) != NULL) {
            void* text = UIWidgetFindChild(w, 0x29DD4A4E);
            if (text != NULL) {
                if (sCountdown == 0.0f) fn_801F5724(0x2000);
                char buf[0x20];
                SysSprintf(buf, "0:%0.02d", (s32)sCountdown);
                UISetText(text, buf, 1);
            }
        }
    }

    void* panel = ScreenSetFindWidget(set, W_PANEL);
    if (panel == NULL || UIWidgetFindChild(panel, W_PANEL_ROOT) == NULL) return;

    void* bar;
    if (sConnecting == 1 && UIWidgetFindChild(panel, 0xDE4906B0) != NULL &&
        (bar = UIWidgetFindChild(panel, 0x25E15199)) != NULL) {
        sConnectTime += gFrameTime;
        UISetProgress(bar, sConnectTime / CONNECT_SECONDS);
        if (sConnectTime / CONNECT_SECONDS >= 1.0f && !IsShowingError()) BuildMonsters();
    }

    if (UIWidgetFindChild(panel, 0x7CC42DED) == NULL) return;
    void* theirName = UIWidgetFindChild(panel, 0xC8487776);
    void* ourName = UIWidgetFindChild(panel, W_NAMES);
    if (theirName == NULL || ourName == NULL) return;
    if (gAlertPlayerWord != 0x40)
        UISetText(theirName, (const char*)(gPlayerNames + 0x340 + gAlertPlayerWord * 32), 1);
    UISetText(ourName, gLocalMonsterName, 1);
}
