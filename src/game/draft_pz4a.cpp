// Small functions: render/HUD/audio/pad-connect helpers (pz4a batch).
#include "types.h"

extern "C" {
void AudioSetPaused(void* audio, int paused);                   // 0x801FF708
void HudUpdateScreens(void* screens, f32 dt);                   // 0x801BB5BC
void OSReport(const char* fmt, ...);
s32  TriggerDoAction(void* action, void* a1, void* a2);         // 0x801E0054
u64  fn_801425DC(u8* timer);                                    // 0x801425DC (64-bit timer read)
void fn_8015BBC0(s32 a);                                        // 0x8015BBC0
void fn_8015E7C8(void* unusedR3, s32 unusedR4, s32 unusedR5, s32 unusedR6, s32 value);                                    // 0x8015E7C8
void fn_801B9278(u8* p, f32 dt);                                      // 0x801B9278 (arg = *(hud + 4))
void WPADSetDataFormat(s32 chan, s32 a);                              // 0x80272AB0
s32  WPADControlSpeaker(s32 chan, s32 a, void* cb);                    // 0x802737A8
void fn_80202EFC(s32 chan, s32 result);                         // 0x80202EFC (pad connect result callback)

extern u8 gAudio[];
extern void* gStringTable2;                                     // 0x805FB90C
extern u8 lbl_8051DAD0[];
extern u8 lbl_8051E680[];                                       // 0x28-byte records
extern u8 lbl_805F98C1;
extern u8 lbl_805FB594[];                                       // render thread state
extern u8 lbl_805FB768[];                                       // timer object
extern u32 lbl_805FB928;                                        // accumulated time, high word
extern u32 lbl_805FB92C;                                        // accumulated time, low word
extern u8 lbl_805FB931;                                         // timer running flag
extern u32 lbl_805FB938;                                        // start stamp, high word
extern u32 lbl_805FB93C;                                        // start stamp, low word
extern s8 lbl_805FBA34;
extern s8 lbl_805FBAC4;
}

extern "C" {

// 0x8015D578
// Runs fn_8015E7C8 on the render thread state, then fn_8015BBC0(0).
void fn_8015D578(void) {
    fn_8015E7C8(lbl_805FB594, 0, 0, 0, 0 /* stale regs */);
    fn_8015BBC0(0);
}

// 0x801A9EC8
// Stores a 3-float vector at +0x14C/+0x150/+0x154; returns 0.
s32 fn_801A9EC8(u8* obj, f32 x, f32 y, f32 z) {
    *(f32*)(obj + 0x14C) = x;
    *(f32*)(obj + 0x150) = y;
    *(f32*)(obj + 0x154) = z;
    return 0;
}

// 0x80174A24
// Writes v to *p, then writes the byte-swapped v (asm stores both, in that order).
void fn_80174A24(u8* p, u32 v) {
    u32 s = (v << 24) | ((v & 0xFF00u) << 8) | ((v >> 8) & 0xFF00u) | (v >> 24);
    *(u32*)(p + 0x0) = v;
    *(u32*)(p + 0x0) = s;
}

// 0x801BB338
// Same as fn_80174A24 but at offset +8.
void fn_801BB338(u8* p, u32 v) {
    u32 s = (v << 24) | ((v & 0xFF00u) << 8) | ((v >> 8) & 0xFF00u) | (v >> 24);
    *(u32*)(p + 0x8) = v;
    *(u32*)(p + 0x8) = s;
}

// 0x8018C48C
// Finds the first of up to 3 0x24-byte entries whose +4 word equals key (stops at an
// entry whose +4 is -1); returns a pointer to that +4 word or NULL.
u8* fn_8018C48C(u8* base, u32 key) {
    u8* e = base;
    for (s32 i = 0; i < 3; i++, e += 0x24) {
        u32 id = *(u32*)(e + 0x4);
        if (id == 0xFFFFFFFFu) break;
        if (id == key) return base + i * 0x24 + 4;
    }
    return NULL;
}

// 0x801ACBEC
// If the HUD has a screens object (+0): run fn_801B9278 on +4 when set, then update the screens.
void fn_801ACBEC(u8* hud, f32 dt) {
    if (*(void**)(hud + 0x0) != NULL) {
        void* p = *(void**)(hud + 0x4);
        if (p != NULL) fn_801B9278((u8*)p, 0.0f /* leftover f1 dt */);
        HudUpdateScreens(*(void**)(hud + 0x0), dt);
    }
}

// 0x801F0668
// Starts/stops the play-time stopwatch: on start remember the 64-bit timer stamp, on stop add
// the elapsed ticks to the 64-bit total.
void fn_801F0668(s32 enable) {
    if (enable != 0) {
        if (lbl_805FB931 == 0) {
            u64 t = fn_801425DC(lbl_805FB768);
            lbl_805FB93C = (u32)t;
            lbl_805FB938 = (u32)(t >> 32);
            lbl_805FB931 = (u8)enable;
        }
    } else if (lbl_805FB931 != 0) {
        u64 t = fn_801425DC(lbl_805FB768);
        u64 start = ((u64)lbl_805FB938 << 32) | (u64)lbl_805FB93C;
        u64 total = ((u64)lbl_805FB928 << 32) | (u64)lbl_805FB92C;
        total += t - start;
        lbl_805FB931 = (u8)enable;
        lbl_805FB92C = (u32)total;
        lbl_805FB928 = (u32)(total >> 32);
    }
}

// 0x801BBE40
// Evaluates a packed trigger word (type in bits 27..30, 0 = none): type 3 looks the value up in
// the table at hud+0x54 (0x3FFFF when out of range); then runs TriggerDoAction on it.
s32 fn_801BBE40(u32* cond, u8* hud, s32 a) {
    if (((*cond >> 27) & 0xF) == 0) return 0;
    gStringTable2 = *(void**)(*(u8**)(hud + 0x0) + 0xC);
    u32 v = *cond;
    if (((v >> 27) & 0xF) == 3) {
        u32 idx = v & 0x3FFFF;
        u32* table = *(u32**)(hud + 0x54);
        if (table != NULL && idx < table[0]) v = table[idx + 1];
        else v = 0x3FFFF;
    }
    u32 sp8 = v;
    return TriggerDoAction(&sp8, *(void**)(hud + 0x10), NULL);
}

// 0x80203184
// Unpauses the audio; counts down gAudio+0x2B4; at 0 sets the flag byte gAudio+0x29 (when
// lbl_805F98C1 is set only if lbl_8051DAD0+0xB4 == 2). Then sets lbl_805FBAC4.
void fn_80203184(void) {
    AudioSetPaused(gAudio, 0);
    if (*(s32*)(gAudio + 0x2B4) > 0) *(s32*)(gAudio + 0x2B4) = *(s32*)(gAudio + 0x2B4) - 1;
    if (*(s32*)(gAudio + 0x2B4) == 0) {
        if (lbl_805F98C1 != 0) {
            if (*(u32*)(lbl_8051DAD0 + 0xB4) == 2u) gAudio[0x29] = 1;
        } else {
            gAudio[0x29] = 1;
        }
    }
    lbl_805FBAC4 = 1;
}

// 0x80202FFC
// Pad connect callback (chan, status): logs, and on connect (status >= 0) with lbl_805FBA34 clear
// resets the channel and requests the connect result via fn_80202EFC.
void fn_80202FFC(s32 chan, s32 status) {
    const char* what = "connect";
    if (status < 0) what = "disconnect";
    OSReport("ConnectCallback(%d) : %s\n", chan, what);
    if (status >= 0 && lbl_805FBA34 == 0) {
        WPADSetDataFormat(chan, 0);
        u8* rec = lbl_8051E680 + chan * 0x28;
        *(s32*)(rec + 0x24) = 0;
        *(s32*)(rec + 0x24) = 1;
        WPADControlSpeaker(chan, 1, (void*)fn_80202EFC);
    }
}

}  // extern "C"
