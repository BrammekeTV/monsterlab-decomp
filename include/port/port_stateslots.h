// PC port: the player's savestate slots (src/port/port_stateslots.cpp).
#ifndef PORT_STATESLOTS_H
#define PORT_STATESLOTS_H
#include "types.h"

enum { PORT_STATEKEY_SAVE, PORT_STATEKEY_LOAD, PORT_STATEKEY_PREV, PORT_STATEKEY_NEXT, PORT_STATEKEY_COUNT };
enum { PORT_STATE_SLOTS = 10, PORT_THUMB_W = 160, PORT_THUMB_H = 90 };
// PortStateWrite (port_state.cpp): RESUMED is the return in the process that loaded the state; LATER: try
// again at a later frame.
enum { PORT_STATE_SAVED = 1, PORT_STATE_RESUMED = 2, PORT_STATE_MOVIE = -1, PORT_STATE_LATER = -2, PORT_STATE_FAILED = -3 };

extern int gPortStateKeys[PORT_STATEKEY_COUNT];                 // Windows virtual-key codes, 0: none ([savestates] in monsterlab.ini)
extern const char* const kPortStateKeyNames[PORT_STATEKEY_COUNT];

struct PortStateSlotFile {
    char when[24];                      // local time the state was written
    unsigned long long stamp;           // changes when the file does
    const char* problem;                // why it cannot be loaded, or NULL
};

void PortStateSlotsFrame(s32 frame);    // PortFrameDone: keys, requests, the save or load itself
BOOL PortStateAddKeyArg(const char* value);   // --state-key save|load|prev|next@FRAME
void PortStateRequest(int action, int slot);  // the settings screen's buttons
int  PortStateSlot(void);
BOOL PortStateSlotInfo(int slot, PortStateSlotFile* out);       // FALSE: the slot is empty
BOOL PortStateSlotThumb(int slot, u8* rgba);                    // PORT_THUMB_W x PORT_THUMB_H RGBA
const char* PortStateMessage(void);     // the short on-screen message, or NULL

s32  PortStateWrite(const char* path, s32 frame);               // port_state.cpp
const char* PortStateCheck(const char* path);                   // NULL: this exe can load the file
BOOL PortStateBusy(void);                                       // the game has a save file open

BOOL PortGfxThumbnail(u32 w, u32 h, u8* rgba);                  // port_gfx_d3d11.cpp: the shown picture, scaled down
BOOL PortGfxWindowPlace(struct tagRECT* r);                     // where the window is (before fullscreen)
BOOL PortGfxIsFullscreen(void);
void PortGfxShowWindow(void);           // a process started by a load keeps its window hidden until now
#endif
