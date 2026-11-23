// System alert boxes (disc, controller, save and network errors): a table from alert hash to
// title/body text and button style.
// Original: ShowAlertBox 0x80140EF8 (auto_03_8013CF8C_text.s area)
#include <stdio.h>

#include "types.h"

extern "C" {
void        fn_801D659C(void);
void*       AlertGetScreenSets(void);                          // 0x8013F38C
const char* GetText(s32 id);                                  // 0x801401FC
u32         AlertGetLastError(void);                           // 0x80140B58
void        AlertShowScreen(const char* title, const char* body, s32 style);   // 0x80141CA4
s32         AlertGetTimer(void);                               // 0x80142460
s32        SysSprintf(char* buf, const char* fmt, ...);       // 0x80142EFC
ScriptValue IsThisPAL(void);
BOOL        IsShowingError(void);
void        UIPlayAnim(void* el, u32 anim);                    // 0x8019BE40
void        ScreenSetsActivate(void* sets, u32 hash);          // 0x8018D70C
}

extern u8   gPlayerNames[];          // 0x80556E80 (+0x340: 32-byte names)
extern u8   gAlertPlayer;            ORIG(0x805F902C)
extern u32  gAlertPlayerWord;        ORIG(0x805F9030)

enum AlertKind {
    ALERT_PLAIN,        // title + body
    ALERT_NAMED,        // body is a format with a player name (index gAlertPlayer)
    ALERT_NAMED_WORD,   // same, index gAlertPlayerWord, skipped when 0x40
    ALERT_TIMED,        // title is "<text> <seconds>", body differs on PAL / NTSC
    ALERT_TIMED_OPT,    // same, seconds left out when 0
};

struct AlertDef {
    u32 hash;
    s32 kind;
    s16 body;           // text id (PAL text for timed alerts)
    s16 bodyNtsc;
    s16 title;
    s16 style;
};

static const AlertDef kAlerts[] = {
    { 0x0D84AA48, ALERT_TIMED     , 0x1066, 0x1065, 0x5A5,  2 },
    { 0x0FDD8F3A, ALERT_TIMED     , 0x102E, 0x102D, 0x5A5,  2 },
    { 0x1871985C, ALERT_PLAIN     , 0x060E, 0x060E, 0x5F0,  5 },
    { 0x1A12B881, ALERT_PLAIN     , 0x05D9, 0x05D9, 0x68C,  5 },
    { 0x24B2E76C, ALERT_PLAIN     , 0x05B3, 0x05B3, 0x68C,  1 },
    { 0x2691BCF8, ALERT_NAMED_WORD, 0x0554, 0x05F0, 0x5F0,  5 },
    { 0x26D671BC, ALERT_PLAIN     , 0x03E3, 0x03E3, 0x5A5,  1 },
    { 0x2D305628, ALERT_PLAIN     , 0x05CD, 0x05CD, 0x68C,  1 },
    { 0x340550DD, ALERT_TIMED     , 0x1083, 0x1082, 0x5A5,  2 },
    { 0x4DE1C242, ALERT_NAMED     , 0x0590, 0x068C, 0x68C,  5 },
    { 0x5639D127, ALERT_TIMED     , 0x1041, 0x1041, 0x5A5,  2 },
    { 0x5A9872D0, ALERT_TIMED     , 0x1025, 0x1025, 0x5A5,  2 },
    { 0x6DECD383, ALERT_PLAIN     , 0x060B, 0x060B, 0x68C,  5 },
    { 0x7129CD52, ALERT_PLAIN     , 0x05E1, 0x05E1, 0x5A5,  1 },
    { 0x7A76413A, ALERT_PLAIN     , 0x0658, 0x0658, 0x5F0,  1 },
    { 0x7FF14FE1, ALERT_PLAIN     , 0x0561, 0x0561, 0x68C,  5 },
    { 0x84776C87, ALERT_NAMED     , 0x0637, 0x05F0, 0x5F0,  1 },
    { 0x8554F56C, ALERT_TIMED     , 0x107A, 0x1079, 0x5A5,  2 },
    { 0x89FF1BE2, ALERT_PLAIN     , 0x0474, 0x0474, 0x5A5,  1 },
    { 0x8A58F9B5, ALERT_PLAIN     , 0x107F, 0x107F, 0x68C, 16 },
    { 0x8DF96546, ALERT_NAMED     , 0x0628, 0x05F0, 0x5F0,  1 },
    { 0x8F2E6505, ALERT_TIMED     , 0x105C, 0x105B, 0x5A5,  2 },
    { 0x9E949532, ALERT_NAMED     , 0x05B2, 0x05F0, 0x5F0,  1 },
    { 0xA60221EA, ALERT_PLAIN     , 0x0635, 0x0635, 0x5F0,  1 },
    { 0xA783B2A1, ALERT_PLAIN     , 0x05E9, 0x05E9, 0x5F0,  1 },
    { 0xA9E14CD9, ALERT_PLAIN     , 0x0672, 0x0672, 0x5F0, 20 },
    { 0xB4F489C1, ALERT_NAMED     , 0x0601, 0x068C, 0x68C,  5 },
    { 0xB62F0997, ALERT_PLAIN     , 0x068B, 0x068B, 0x5F0, 20 },
    { 0xBC882260, ALERT_PLAIN     , 0x0659, 0x0659, 0x5F0,  5 },
    { 0xBD65120A, ALERT_TIMED     , 0x107D, 0x107D, 0x5A5,  0 },
    { 0xC0189F1E, ALERT_TIMED_OPT , 0x1028, 0x1027, 0x5A5,  2 },
    { 0xC0D8384A, ALERT_PLAIN     , 0x0668, 0x0668, 0x5F0,  1 },
    { 0xC6D953F2, ALERT_PLAIN     , 0x03E3, 0x03E3, 0x5F0,  1 },
    { 0xD21C7E5D, ALERT_PLAIN     , 0x0580, 0x0580, 0x5A5,  1 },
    { 0xD48D7CA1, ALERT_TIMED     , 0x1061, 0x1061, 0x5A5,  2 },
    { 0xE6C761FE, ALERT_PLAIN     , 0x060F, 0x060F, 0x5F0,  1 },
    { 0xEB4BCABF, ALERT_TIMED     , 0x1077, 0x1077, 0x5A5,  2 },
    { 0xEB7746BD, ALERT_PLAIN     , 0x0604, 0x0604, 0x68C,  1 },
    { 0xF08D5746, ALERT_NAMED_WORD, 0x061F, 0x068C, 0x68C,  1 },
    { 0xF177C7E8, ALERT_NAMED     , 0x0638, 0x05F0, 0x5F0,  1 },
    { 0xF4B22FF2, ALERT_PLAIN     , 0x057B, 0x057B, 0x65B,  5 },
};

#define ALERT_DISC_SPECIAL  0x50D945A4

static inline const char* PlayerName(u32 i) { return (const char*)(gPlayerNames + 0x340 + i * 32); }

extern "C" void ShowAlertBox(u32 hash);
// 0x80140EF8
void ShowAlertBox(u32 hash) {
    fn_801D659C();
    if (*(void**)((u8*)AlertGetScreenSets() + 0x74) != NULL)
        UIPlayAnim(*(void**)((u8*)AlertGetScreenSets() + 0x74), 0xF5A9CE6C);

    char title[0x40], body[0x100];
    if (hash == ALERT_DISC_SPECIAL) {
        if (AlertGetLastError() != 0xAEDD8AD0) AlertShowScreen(GetText(0x5A5), GetText(0x331), 1);
        else if (!IsShowingError())            AlertShowScreen(GetText(0x5A5), GetText(0x596), 1);
        ScreenSetsActivate(AlertGetScreenSets(), 0x7A1060A7);
        return;
    }

    const AlertDef* a = NULL;
    for (u32 i = 0; i < sizeof(kAlerts) / sizeof(kAlerts[0]); i++)
        if (kAlerts[i].hash == hash) { a = &kAlerts[i]; break; }
    if (a == NULL) return;

    switch (a->kind) {
    case ALERT_PLAIN:
        AlertShowScreen(GetText(a->title), GetText(a->body), a->style);
        break;
    case ALERT_NAMED:
        SysSprintf(body, GetText(a->body), PlayerName(gAlertPlayer));
        AlertShowScreen(GetText(a->title), body, a->style);
        break;
    case ALERT_NAMED_WORD:
        body[0] = 0;
        if (gAlertPlayerWord != 0x40) SysSprintf(body, GetText(a->body), PlayerName(gAlertPlayerWord));
        AlertShowScreen(GetText(a->title), body, a->style);
        break;
    case ALERT_TIMED:
    case ALERT_TIMED_OPT: {
        s32 secs = (s32)AlertGetTimer();
        if (a->kind == ALERT_TIMED_OPT && secs == 0) SysSprintf(title, "%s", GetText(a->title));
        else                                          SysSprintf(title, "%s %d", GetText(a->title), secs);
        AlertShowScreen(title, GetText(IsThisPAL() ? a->body : a->bodyNtsc), a->style);
        break;
    }
    }
    ScreenSetsActivate(AlertGetScreenSets(), 0x7A1060A7);
}
