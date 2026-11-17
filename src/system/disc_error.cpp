// Disc error handling: the "Please insert the Game Disc" / "could not be read" screens,
// shown by blocking the game until the drive recovers, plus reset/power handling while
// such a message is up.
// Original object: .text 0x80010AC0 - 0x80011018 (auto_03_80010AC0_text.s)
//
// PC port note: there is no disc, so the platform layer never reports a drive error and
// this code never runs; it is kept for completeness.
#include "revolution.h"
#include "system/sys.h"

extern "C" {
s32   DVDGetDriveStatus(void);                          // 0x8025394C
void  WPADControlMotor(s32 chan, u32 command);          // 0x80272E3C
void  SetRenderEnabled(BOOL on);
void  AudioSetPaused(void* audio, int paused);          // 0x801FF708
void  AudioSuspend(void* audio);                        // 0x801FD8EC
void  AudioResume(void* audio);                         // 0x801FD904
void  AudioStopAll(void);                               // 0x801FD1F0
void  PadSuspend(s32 chan);                             // 0x801B2CE4
void  PadResume(s32 chan);                              // 0x801B2D98
void  SleepMicroseconds(int unused, u32 us);            // 0x80142818
f32   TimerGetSeconds(void* timer);                     // 0x80142538
void  TimerReset(void* timer);                          // 0x80142680
s32   GetRegion(void);                                // 0x8001425C: 0 = NA, 1 = EU
BOOL  fn_801F56FC(s32);  void fn_801F38E8(s32); void fn_8013BD7C(void); void fn_801F5724(s32);
void  ShowSystemMessage(const char* msg, BOOL (*poll)(void));   // 0x801EE588
s32   GameHasSysEvent(void);                            // 0x8000CF8C
s32   GameGetSysEvent(void);                            // 0x8000CFA0
void  SysRequestExit(int mode);                         // 0x80142DE8
void  BaseObjectDtor(void* member, s32 flags);          // 0x801E7BC4
void  operator_delete(void* p);                         // 0x801D426C
}

enum {
    DVD_STATE_FATAL_ERROR = -1, DVD_STATE_END = 0, DVD_STATE_BUSY = 1,
    DVD_STATE_NO_DISK = 4, DVD_STATE_WRONG_DISK = 6, DVD_STATE_RETRY = 11,
};

// Localized messages, [kind][language]: no disc, wrong disc, cannot read, fatal.
extern const char* const sDiscErrorMessages[4][10];    ORIG(0x80354618)
extern const char        sDiscReadErrorAltEN[];        ORIG(0x803546E0)
extern const char        sEmptyString[];               ORIG(0x805F81F8)

extern u8   gAudio[];                ORIG(0x8047C7F0)  // +0x2AC: audio running
extern s32  gTextLanguage;           // 0x805F81B4
extern s32  lbl_805FB970, lbl_805FBB64;
extern void* gStringTable;           // 0x805FB910
extern const char lbl_805F8200[];
static volatile s32 sRetraceFlag;    ORIG(0x805FBA88)   // set by the retrace interrupt

static s32  sLastDriveStatus;        ORIG(0x805FAEE0)
static f32  sStatusChangeTime;       ORIG(0x805FAEE4)
static u8   sStatusChanged;          ORIG(0x805FAEE8)
static u8   sErrorTimer[8];          ORIG(0x805FAEF0)

static inline BOOL IsDriveError(s32 status) {
    return status == DVD_STATE_FATAL_ERROR || status == DVD_STATE_NO_DISK ||
           status == DVD_STATE_WRONG_DISK || status == DVD_STATE_RETRY;
}

// 0x80010AC0
// Deleting destructor of a small object holding a member at +0x40.
void* fn_80010AC0(void* self, s32 flags) {
    if (self != NULL) {
        BaseObjectDtor((u8*)self + 0x40, -1);
        if (flags > 0) operator_delete(self);
    }
    return self;
}

// 0x80010B1C
void* fn_80010B1C(void* self, s32 flags) {
    if (self != NULL && flags > 0) operator_delete(self);
    return self;
}

// 0x80010B5C
// Poll function for the disc error message: returns TRUE when the message should close
// (drive state changed, recovered, or 10 s passed in a transitional state).
static BOOL DiscErrorPoll(void) {
    BOOL close = FALSE;
    s32 status = DVDGetDriveStatus();
    if (status != sLastDriveStatus && !sStatusChanged) {
        sStatusChanged = TRUE;
        sStatusChangeTime = TimerGetSeconds(sErrorTimer);
    }

    if (sStatusChanged) {
        if (IsDriveError(status) || status == DVD_STATE_END) close = TRUE;
        else if (TimerGetSeconds(sErrorTimer) > 10.0f + sStatusChangeTime) close = TRUE;
    }

    if (GameHasSysEvent()) {
        SetRenderEnabled(FALSE);
        if (gAudio[0x2AC]) AudioStopAll();
        switch (GameGetSysEvent()) {
        case 1: OSRestart(0); break;
        case 2: OSReturnToMenu(); break;
        case 3: OSShutdownSystem(); break;
        }
        close = TRUE;
    }
    return close;
}

// 0x80010CA4
static BOOL NeverClose(void) {
    return FALSE;
}

static inline const char* PickMessage(int kind) {
    const char* msg = sDiscErrorMessages[kind][gTextLanguage];
    GetRegion();
    if (msg == NULL) msg = sDiscErrorMessages[kind][0];
    if (msg == NULL) msg = sEmptyString;
    return msg;
}

// 0x80010CAC
// Called when a disc read fails: freeze audio and controllers, then show messages
// until the drive is readable again.
static void DiscErrorHandler(void) {
    if (gAudio[0x2AC]) {
        AudioSetPaused(gAudio, TRUE);
        AudioSuspend(gAudio);
        for (s32 i = 0; i < 4; i++) PadSuspend(i);
        // Let four retraces pass so the audio DMA drains.
        for (int n = 4; n != 0; n--) {
            sRetraceFlag = 0;
            DCFlushRange((void*)&sRetraceFlag, sizeof(sRetraceFlag));
            while (sRetraceFlag == 0) SleepMicroseconds(0, 1000);
        }
    }

    if (lbl_805FB970 != 0 && !fn_801F56FC(2)) {
        if (lbl_805FBB64 != 0) fn_801F38E8(1);
        else                   fn_8013BD7C();
        fn_801F5724(0x110);
    }

    for (s32 chan = 0; chan < 4; chan++) WPADControlMotor(chan, 0);   // stop rumble

    s32 status = DVDGetDriveStatus();
    while (IsDriveError(status)) {
        sLastDriveStatus = status;
        TimerReset(sErrorTimer);
        sStatusChangeTime = 0.0f;
        sStatusChanged = FALSE;

        switch (status) {
        case DVD_STATE_NO_DISK:
            ShowSystemMessage(PickMessage(0), DiscErrorPoll);
            break;
        case DVD_STATE_WRONG_DISK:
            ShowSystemMessage(PickMessage(1), DiscErrorPoll);
            break;
        case DVD_STATE_RETRY: {
            const char* msg = sDiscErrorMessages[2][gTextLanguage];
            if (GetRegion() && gTextLanguage == 0) msg = sDiscReadErrorAltEN;
            if (msg == NULL) msg = sDiscErrorMessages[2][0];
            if (msg == NULL) msg = sEmptyString;
            ShowSystemMessage(msg, DiscErrorPoll);
            break;
        }
        }
        status = DVDGetDriveStatus();
    }

    for (s32 i = 0; i < 4; i++) PadResume(i);
    AudioSetPaused(gAudio, FALSE);
    AudioResume(gAudio);
}

// 0x80010F64
// Registered with the file system as the read-error callback.
extern "C" s32 fn_80010F64(void) {
    DiscErrorHandler();
    return 0;
}

// 0x80010F88
// Registered with the save system: on error -4 show string #0x1080 forever.
extern "C" s32 fn_80010F88(s32 error) {
    if (error == -4) {
        struct StringTable { const char** strings; u8 pad[8]; u32 count; };
        StringTable* st = (StringTable*)gStringTable;
        const char* msg = (st->strings != NULL && st->count > 0x1080) ? st->strings[0x1080] : lbl_805F8200;
        ShowSystemMessage(msg, NeverClose);
    }
    return 0;
}

// 0x80010FE8
void SysRequestReset(void) {
    SysRequestExit(0);
}

// 0x80010FF0
void DiscErrorHandlerExt(void) {
    DiscErrorHandler();
}
