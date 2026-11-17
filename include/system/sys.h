#pragma once
#include "types.h"

enum SysExitMode {
    SYS_EXIT_NONE = 0,
    SYS_EXIT_RESTART = 1,   // reset button: OSRestart
    SYS_EXIT_MENU = 2,      // return to Wii Menu
    SYS_EXIT_SHUTDOWN = 3,  // power button
};

enum SysEvent {
    SYS_EVENT_RESET = 1,
    SYS_EVENT_POWER = 3,
};

typedef void (*SysEventCallback)(int event);

extern u8 gSysExitEnabled;
extern SysEventCallback gSysEventCallback;

void        SysInit(void);
void*       SysAllocMEM2(u32 size);
BOOL        SysFreeMEM2(void* ptr);
void        SysSetExitMode(SysExitMode mode);
SysExitMode SysGetExitMode(void);
SysExitMode SysGetRequestedExitMode(void);

int GameMain(int argc, char** argv);
