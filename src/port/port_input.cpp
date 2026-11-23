// PC input: keyboard, mouse and XInput gamepads presented to the game as a Wii Remote with a
// Nunchuk on channel 0, behind KPADRead / WPADProbe. No original code.
//
// The game reads only the buttons held (KPADStatus +0x00), the accelerometers, the pointer
// (+0x20, valid flag +0x5E), the horizon, the extension type and error, and the Nunchuk's stick
// (+0x60) in fn_801479D0, which turns them into its own per-channel register block.
//
// All bindings live in gPortBindings (include/port/port_input.h) so a settings screen can edit
// them. The keyboard and mouse count only while the port's window has the focus; headless runs
// get input from --press (scripted presses, for tests): --press A@2400 or --press A@2400-2410
// (button name from the table, frame numbers as counted by PortFrameDone).
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <xinput.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/port.h"
#include "port/port_gx.h"
#include "port/port_input.h"
#include "port/port_overlay.h"

// WPAD button bits as KPADStatus.hold carries them.
enum {
    W_LEFT = 0x0001, W_RIGHT = 0x0002, W_DOWN = 0x0004, W_UP = 0x0008, W_PLUS = 0x0010,
    W_2 = 0x0100, W_1 = 0x0200, W_B = 0x0400, W_A = 0x0800, W_MINUS = 0x1000,
    W_Z = 0x2000, W_C = 0x4000, W_HOME = 0x8000,
};

// Extra "pad" inputs beyond the XInput button bits: the analog triggers.
#define PAD_LT 0x10000u
#define PAD_RT 0x20000u

PortBinding gPortBindings[] = {
    // name     action             key 1        key 2        mouse  XInput
    {"A",       PORT_IN_BUTTON, W_A,     VK_RETURN,   VK_SPACE,    1, XINPUT_GAMEPAD_A},
    {"B",       PORT_IN_BUTTON, W_B,     VK_BACK,     'X',         2, XINPUT_GAMEPAD_B},
    {"1",       PORT_IN_BUTTON, W_1,     '1',         0,           0, XINPUT_GAMEPAD_X},
    {"2",       PORT_IN_BUTTON, W_2,     '2',         0,           0, XINPUT_GAMEPAD_Y},
    {"PLUS",    PORT_IN_BUTTON, W_PLUS,  VK_OEM_PLUS, VK_ADD,      0, XINPUT_GAMEPAD_START},
    {"MINUS",   PORT_IN_BUTTON, W_MINUS, VK_OEM_MINUS, VK_SUBTRACT, 0, XINPUT_GAMEPAD_BACK},
    {"HOME",    PORT_IN_BUTTON, W_HOME,  VK_HOME,     0,           0, 0},
    {"UP",      PORT_IN_BUTTON, W_UP,    VK_UP,       0,           0, XINPUT_GAMEPAD_DPAD_UP},
    {"DOWN",    PORT_IN_BUTTON, W_DOWN,  VK_DOWN,     0,           0, XINPUT_GAMEPAD_DPAD_DOWN},
    {"LEFT",    PORT_IN_BUTTON, W_LEFT,  VK_LEFT,     0,           0, XINPUT_GAMEPAD_DPAD_LEFT},
    {"RIGHT",   PORT_IN_BUTTON, W_RIGHT, VK_RIGHT,    0,           0, XINPUT_GAMEPAD_DPAD_RIGHT},
    {"Z",       PORT_IN_BUTTON, W_Z,     VK_LSHIFT,   'E',         0, XINPUT_GAMEPAD_RIGHT_SHOULDER | PAD_RT},
    {"C",       PORT_IN_BUTTON, W_C,     VK_LCONTROL, 'Q',         0, XINPUT_GAMEPAD_LEFT_SHOULDER | PAD_LT},
    // The Nunchuk stick (the XInput left stick is read as an analog stick besides these).
    {"STICK_UP",    PORT_IN_STICK_UP,    0, 'W', 0, 0, 0},
    {"STICK_DOWN",  PORT_IN_STICK_DOWN,  0, 'S', 0, 0, 0},
    {"STICK_LEFT",  PORT_IN_STICK_LEFT,  0, 'A', 0, 0, 0},
    {"STICK_RIGHT", PORT_IN_STICK_RIGHT, 0, 'D', 0, 0, 0},
};
const int gPortBindingCount = sizeof(gPortBindings) / sizeof(gPortBindings[0]);

HWND PortGfxWindow(void);
s32  PortFrameCount(void);

// ------------------------------------------------------------------ scripted presses (--press)
struct ScriptedPress { u32 bits; s32 from, to; f32 sx, sy; };
static ScriptedPress sPress[32];
static int sPressCount;

static const PortBinding* FindBinding(const char* name, size_t len) {
    for (int i = 0; i < gPortBindingCount; i++)
        if (strlen(gPortBindings[i].name) == len && !_strnicmp(gPortBindings[i].name, name, len)) return &gPortBindings[i];
    return NULL;
}

BOOL PortInputAddPress(const char* spec) {
    const char* at = strchr(spec, '@');
    if (!at || sPressCount >= 32) return FALSE;
    const PortBinding* b = FindBinding(spec, (size_t)(at - spec));
    if (!b) return FALSE;
    ScriptedPress& p = sPress[sPressCount];
    memset(&p, 0, sizeof(p));
    char* end;
    p.from = strtol(at + 1, &end, 10);
    p.to = (*end == '-') ? strtol(end + 1, NULL, 10) : p.from + 5;     // a press lasts 6 frames by default
    switch (b->action) {
    case PORT_IN_BUTTON:      p.bits = b->wiiBits; break;
    case PORT_IN_STICK_UP:    p.sy = 1.0f; break;
    case PORT_IN_STICK_DOWN:  p.sy = -1.0f; break;
    case PORT_IN_STICK_LEFT:  p.sx = -1.0f; break;
    case PORT_IN_STICK_RIGHT: p.sx = 1.0f; break;
    }
    sPressCount++;
    return TRUE;
}

// --pointer X,Y@FRAME: from that frame on the pointer is at output pixel X,Y (mapped exactly as the mouse;
// a later entry moves it), for headless tests.
struct ScriptedPointer { s32 from; f32 wx, wy; };
static ScriptedPointer sPointer[32];
static int sPointerCount;

BOOL PortInputAddPointer(const char* spec) {
    const char* at = strchr(spec, '@');
    if (!at || sPointerCount >= 32) return FALSE;
    char* e1; char* e2;
    f32 x = (f32)strtod(spec, &e1);
    if (*e1 != ',') return FALSE;
    f32 y = (f32)strtod(e1 + 1, &e2);
    if (e2 != at) return FALSE;
    ScriptedPointer& p = sPointer[sPointerCount++];
    p.from = atoi(at + 1);
    p.wx = x; p.wy = y;      // output pixels; divided by the output size when sampled (--res may follow)
    return TRUE;
}

// ------------------------------------------------------------------ XInput (loaded on demand)
typedef DWORD (WINAPI* XInputGetStateFn)(DWORD, XINPUT_STATE*);
static XInputGetStateFn sXInputGetState;
static BOOL sXInputTried;

static BOOL ReadPad(XINPUT_STATE* st) {
    if (!sXInputTried) {
        sXInputTried = TRUE;
        HMODULE m = LoadLibraryA("xinput1_4.dll");
        if (!m) m = LoadLibraryA("xinput9_1_0.dll");
        if (m) sXInputGetState = (XInputGetStateFn)GetProcAddress(m, "XInputGetState");
    }
    if (!sXInputGetState) return FALSE;
    for (DWORD i = 0; i < 4; i++) {
        memset(st, 0, sizeof(*st));
        if (sXInputGetState(i, st) == ERROR_SUCCESS) return TRUE;   // the first pad connected
    }
    return FALSE;
}

static f32 Axis(SHORT v, SHORT dead) {
    if (v > -dead && v < dead) return 0.0f;
    f32 f = (v < 0) ? (v + dead) / (32768.0f - dead) : (v - dead) / (32767.0f - dead);
    return f < -1.0f ? -1.0f : f > 1.0f ? 1.0f : f;
}

// ------------------------------------------------------------------ the remote's state
struct RemoteState { u32 hold; f32 sx, sy; f32 px, py; BOOL pointerValid; };
static f32  sPadPx, sPadPy;          // the pointer moved by the right stick
static BOOL sPadPointer;

static void Sample(RemoteState* r) {
    memset(r, 0, sizeof(*r));
    s32 frame = PortFrameCount();
    for (int i = 0; i < sPressCount; i++) {
        if (frame < sPress[i].from || frame > sPress[i].to) continue;
        r->hold |= sPress[i].bits;
        if (sPress[i].sx) r->sx = sPress[i].sx;
        if (sPress[i].sy) r->sy = sPress[i].sy;
    }

    {
        s32 best = -1;
        for (int i = 0; i < sPointerCount; i++)
            if (frame >= sPointer[i].from && (best < 0 || sPointer[i].from >= sPointer[best].from)) best = i;
        if (best >= 0) {
            PortGfxPointer(sPointer[best].wx / (gPortSettings.width > 0 ? gPortSettings.width : 1920), sPointer[best].wy / (gPortSettings.height > 0 ? gPortSettings.height : 1080), &r->px, &r->py);
            r->pointerValid = TRUE;
        }
    }

    HWND wnd = PortGfxWindow();
    BOOL focus = wnd && GetForegroundWindow() == wnd && !gPortGuiOpen;   // the settings screen has the keyboard and mouse
    XINPUT_STATE pad;
    BOOL havePad = wnd && ReadPad(&pad);     // headless runs stay deterministic
    u32 padBits = 0;
    if (havePad) {
        padBits = pad.Gamepad.wButtons;
        if (pad.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) padBits |= PAD_LT;
        if (pad.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) padBits |= PAD_RT;
    }
    f32 kx = 0.0f, ky = 0.0f;
    for (int i = 0; i < gPortBindingCount; i++) {
        const PortBinding& b = gPortBindings[i];
        BOOL down = FALSE;
        if (focus) {
            if (b.key[0] && (GetAsyncKeyState(b.key[0]) & 0x8000)) down = TRUE;
            if (b.key[1] && (GetAsyncKeyState(b.key[1]) & 0x8000)) down = TRUE;
            if (b.mouse == 1 && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) down = TRUE;
            if (b.mouse == 2 && (GetAsyncKeyState(VK_RBUTTON) & 0x8000)) down = TRUE;
        }
        if (b.pad && (padBits & b.pad)) down = TRUE;
        if (!down) continue;
        switch (b.action) {
        case PORT_IN_BUTTON:      r->hold |= b.wiiBits; break;
        case PORT_IN_STICK_UP:    ky += 1.0f; break;
        case PORT_IN_STICK_DOWN:  ky -= 1.0f; break;
        case PORT_IN_STICK_LEFT:  kx -= 1.0f; break;
        case PORT_IN_STICK_RIGHT: kx += 1.0f; break;
        }
    }
    if (kx || ky) {
        f32 len = sqrtf(kx * kx + ky * ky);
        r->sx = kx / len; r->sy = ky / len;
    }
    if (havePad) {
        f32 ax = Axis(pad.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        f32 ay = Axis(pad.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        if (ax || ay) { r->sx = ax; r->sy = ay; }
        // The right stick moves the pointer (KPAD's y grows downwards).
        f32 rx = Axis(pad.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        f32 ry = Axis(pad.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
        if (rx || ry) {
            sPadPointer = TRUE;
            sPadPx += rx * 0.03f; sPadPy -= ry * 0.03f;
            sPadPx = sPadPx < -1.0f ? -1.0f : sPadPx > 1.0f ? 1.0f : sPadPx;
            sPadPy = sPadPy < -1.0f ? -1.0f : sPadPy > 1.0f ? 1.0f : sPadPy;
        }
    }

    // The mouse in the window's client area is the IR pointer: -1..1 across, -1 at the top.
    if (focus) {
        POINT pt; RECT rc;
        if (GetCursorPos(&pt) && ScreenToClient(wnd, &pt) && GetClientRect(wnd, &rc) && rc.right > 0 && rc.bottom > 0 &&
            pt.x >= 0 && pt.y >= 0 && pt.x < rc.right && pt.y < rc.bottom) {
            static POINT last = {-1, -1};
            if (pt.x != last.x || pt.y != last.y) sPadPointer = FALSE;  // the mouse moved: it leads again
            last = pt;
            if (!sPadPointer) {
                PortGfxPointer((f32)pt.x / rc.right, (f32)pt.y / rc.bottom, &r->px, &r->py);
                r->pointerValid = TRUE;
            }
        }
    }
    if (sPadPointer) { r->px = sPadPx; r->py = sPadPy; r->pointerValid = TRUE; }
}

extern "C" {

// KPADRead(channel, samples, capacity): the remote's samples since the last call, newest first;
// returns how many (one per call here). A sample is the SDK's KPADStatus (0x84 bytes): hold /
// trigger / release at +0x00/+0x04/+0x08, the accelerometer at +0x0C (y = -1 is the remote lying
// flat) and its length at +0x18, the pointer at +0x20, the horizon at +0x34, dist at +0x48,
// acc_vertical at +0x54, the extension type at +0x5C (1 = Nunchuk), the error at +0x5D (0 = fine,
// -1 = no controller), the pointer-valid flag at +0x5E, then the Nunchuk's stick (+0x60) and
// accelerometer (+0x68) with its length (+0x74). Channels 1 to 3 have nothing connected.
s32 KPADRead(s32 chan, u8* buf, s32 count) {
    static u32 sLastHold;
    if (!buf || count < 1) return 0;
    memset(buf, 0, 0x84);
    if (chan != 0) { buf[0x5C] = 253; buf[0x5D] = 0xFF; return 0; }
    RemoteState r;
    Sample(&r);
    *(u32*)(buf + 0x00) = r.hold;
    *(u32*)(buf + 0x04) = r.hold & ~sLastHold;
    *(u32*)(buf + 0x08) = sLastHold & ~r.hold;
    sLastHold = r.hold;
    *(f32*)(buf + 0x10) = -1.0f;        // acc.y: gravity
    *(f32*)(buf + 0x18) = 1.0f;         // its length
    if (r.pointerValid) {
        *(f32*)(buf + 0x20) = r.px;
        *(f32*)(buf + 0x24) = r.py;
        *(f32*)(buf + 0x48) = 1.0f;     // dist: a metre from the sensor bar
        buf[0x5E] = 2;                  // dpd_valid_fg: both points seen
    }
    *(f32*)(buf + 0x34) = 1.0f;         // horizon.x: level
    *(f32*)(buf + 0x54) = 1.0f;         // acc_vertical.x
    buf[0x5C] = 1;                      // Nunchuk attached
    *(f32*)(buf + 0x60) = r.sx;
    *(f32*)(buf + 0x64) = r.sy;
    *(f32*)(buf + 0x6C) = -1.0f;        // the Nunchuk's acc.y
    *(f32*)(buf + 0x74) = 1.0f;         // its length
    return 1;
}

// WPADProbe(channel, device type): 0 and type 1 = a remote with a Nunchuk; -1 and type 253 = no
// controller on this channel.
s32 WPADProbe(s32 chan, u32* type) {
    if (chan != 0) { if (type) *type = 253; return -1; }
    if (type) *type = 1;
    return 0;
}

}  // extern "C"

int PortSelfTestInput(void) {
    int fails = 0;
    if (!FindBinding("A", 1) || FindBinding("Q", 1)) fails++;
    int saved = sPressCount;
    if (!PortInputAddPress("a@0-1000000") || PortInputAddPress("nope@3")) fails++;
    u8 buf[0x84];
    if (KPADRead(0, buf, 1) != 1 || !(*(u32*)buf & W_A) || buf[0x5C] != 1) fails++;
    if (KPADRead(1, buf, 1) != 0 || buf[0x5D] != 0xFF) fails++;
    sPressCount = saved;
    if (fails) PortLog("self-test FAILED: input (%d)", fails);
    return fails;
}

#endif
