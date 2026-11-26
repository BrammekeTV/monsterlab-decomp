// PC input bindings (port_input.cpp): how keyboard, mouse and XInput gamepads map onto the Wii
// Remote + Nunchuk the game reads through KPADRead. One table, so a settings screen can edit it.
#ifndef PORT_INPUT_H
#define PORT_INPUT_H

#include "types.h"

enum PortInputAction {
    PORT_IN_BUTTON,         // sets wiiBits (WPAD button bits) while held
    PORT_IN_STICK_UP,       // pushes the Nunchuk stick
    PORT_IN_STICK_DOWN,
    PORT_IN_STICK_LEFT,
    PORT_IN_STICK_RIGHT,
    PORT_IN_SWING,          // swings the Wii Remote in circles while held (accelerometer)
};

typedef struct PortBinding {
    const char* name;       // also the name --press uses
    int  action;            // PortInputAction
    u32  wiiBits;
    int  key[2];            // Windows virtual-key codes, 0 = none
    int  mouse;             // 1 = left button, 2 = right button, 0 = none
    u32  pad;               // XINPUT_GAMEPAD_* bits, plus 0x10000 left / 0x20000 right trigger
} PortBinding;

extern PortBinding gPortBindings[];
extern const int   gPortBindingCount;

// --press BUTTON@FRAME[-FRAME]: hold a binding's input during those frames (tests, headless runs).
u32  PortInputPadBits(void);                    // gamepad buttons held now (PortBinding.pad bits; 0 without a pad)
BOOL PortInputAddPress(const char* spec);
BOOL PortInputAddPointer(const char* spec);     // --pointer X,Y@FRAME (output pixels)
void PortInputAddConsolePointer(f32 px, f32 py, s32 frame);   // the same in console pointer values (--bench)

#endif
