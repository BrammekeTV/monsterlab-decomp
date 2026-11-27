#pragma once
// PC port runtime: the Wii address space, the user's main.dol data, traps for code that is not
// decompiled yet, and the "interrupt" scheduling the SDK layer needs. Design: docs/port_runtime.md.
#include "types.h"

// ---- savestates (port_state.cpp, docs/savestates.md) ----
// A state holds the mapped Wii memory, the game threads' stacks and every static in the sections .savd/.savb.
// types.h puts the game code's statics there. Port sources (they include this header) are host state by
// default: nothing of theirs is saved unless it stands between PORT_SAVED_BEGIN and PORT_SAVED_END. A port
// source whose state is nearly all the game's opens with PORT_SAVED_BEGIN after its includes and puts what
// belongs to this process (handles, FILE*, pointers into the C heap, options) between PORT_HOST_BEGIN/END.
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma bss_seg()
#pragma data_seg()
#define PORT_SAVED_BEGIN __pragma(bss_seg(push, ".savb")) __pragma(data_seg(push, ".savd"))
#define PORT_SAVED_END   __pragma(bss_seg(pop)) __pragma(data_seg(pop))
#define PORT_HOST_BEGIN  __pragma(bss_seg(push)) __pragma(data_seg(push)) __pragma(bss_seg()) __pragma(data_seg())
#define PORT_HOST_END    __pragma(bss_seg(pop)) __pragma(data_seg(pop))
#else
#define PORT_SAVED_BEGIN
#define PORT_SAVED_END
#define PORT_HOST_BEGIN
#define PORT_HOST_END
#endif
#define PORT_STACK_BASE  0xA0000000u   // game thread stacks at fixed addresses, so a state can hold them
#define PORT_STACK_SLOT  0x00500000u   // per thread: 1 MB never mapped, then up to 4 MB of stack
#define PORT_STACK_SLOTS 34            // 0 the main thread, 1-32 OSCreateThread threads, 33 unused
#define PORT_HOSTMEM_BASE 0xB0000000u  // PortHostAlloc: port-side buffers a state must hold (catalog copy, the loader's lists, zlib streams)
#define PORT_HOSTMEM_SIZE 0x04000000u
const char* PortSaveDir(void);         // the folder of the save files (port_nand.cpp)
void* PortHostAlloc(u32 size);         // zeroed; a fixed-address heap that states save whole (port_os.cpp)
void  PortHostFree(void* p);
// A saved register set of a thread that waits (or of the thread that saves). Capture returns 0; when a loaded
// state resumes the thread, the same call returns 1 in the new process.
typedef struct PortCtx { u32 esp, ebp, ebx, esi, edi, eip, seh, fcw, mxcsr; } PortCtx;
extern "C" int  __cdecl PortCtxCapture(PortCtx* c);
extern "C" void __cdecl PortCtxResume(const PortCtx* c);
void  PortStackEnter(s32 slot, u32 size, void (__cdecl* fn)(void*), void* arg);   // run fn on the slot's stack; returns when fn does
void  PortStackResume(s32 slot, const PortCtx* c);    // a new thread of a loaded state: go on where the saved one waited
u32   PortStackTop(s32 slot);
void  PortStateInit(void);             // reserves the stacks and the host heap (before anything else runs)
void  PortStateAddArg(const char* opt, const char* value);   // --save-state FILE@FRAME, --load-state FILE
const char* PortStateLoadPath(void);   // the state to start from, or NULL
void  PortStateFrame(s32 frame);       // PortFrameDone: writes the states due at this frame
void  PortStateLoad(void);             // PortPreMain: reads the state and continues its main thread; does not return
// A block of host memory outside the saved sections that a state must hold (the renderer's palette memory).
void  PortStateBlob(const char* name, void* p, u32 size);
// Called in the new process after a state's memory is back and before its threads run: reopen files, fix clocks.
void  PortStateOnLoad(void (*fn)(void));
BOOL  PortStateLoaded(void);           // this run started from a state
s32   PortSchedSave(PortCtx** ctx, u8* active);        // port_runtime.cpp: per stack slot, its register set and whether it lives; returns the caller's slot
const PortCtx* PortStateCtx(s32 slot);                 // loaded state: the registers of that slot's thread, or NULL
BOOL  PortSchedAlive(s32 slot);        // a game thread lives on that stack slot
void  PortSchedAdd(s32 slot);          // a new game thread: it runs when its turn comes (the creator holds the lock)
void  PortSchedBind(s32 slot);         // the calling Windows thread is that game thread
void  PortSchedStart(s32 slot);        // wait for the first turn
void  PortSchedExit(s32 slot);         // the game thread ended: give the lock up for good
void  PortSchedAfterLoad(void);
void  PortOsAfterLoad(void);           // port_os.cpp: starts the saved threads again
void  PortInterruptPass(void);         // deliver what is due (queued interrupts, alarms, retrace, AX frames); lock held
void  PortClockFreeze(void);           // before a state is written: the game time as one number
u32   PortDataHash(void);              // CRC-32 of the main.dol in use
BOOL  PortBinkPlaying(void);

// ---- Wii address space (mapped at the original addresses) ----
#define PORT_MEM1_BASE   0x80000000u
#define PORT_MEM1_SIZE   0x01800000u
#define PORT_MEM2_BASE   0x90000000u
#define PORT_MEM2_SIZE   0x04000000u
#define PORT_HW_BASE     0xCC000000u   // hardware registers; plain memory on PC
#define PORT_HW_SIZE     0x00010000u
#define PORT_TEXT_LO     0x80004000u   // .init + .text of main.dol: holds jumps and traps on PC
#define PORT_TEXT_HI     0x80345AE0u
#define PORT_CTORS       0x80345AE0u   // .ctors: static initialisers, run by OSInit

// ---- tables generated by tools/port_link.py (port_gen.obj) ----
typedef struct PortFunc {     // every function of the original executable, sorted by address
    u32   addr;
    u32   size;
    void* impl;               // linked PC implementation, or NULL
    u32   name;               // offset into gPortNames
} PortFunc;

enum { PORT_DATA_RAW = 0, PORT_DATA_16 = 2, PORT_DATA_32 = 4, PORT_DATA_64 = 8, PORT_DATA_AUTO = 9 };
typedef struct PortDataRun {  // byte order of the initialised data of main.dol
    u32 addr;
    u32 size;
    u32 kind;
} PortDataRun;

extern "C" {
extern const PortFunc    gPortFuncs[];
extern const u32         gPortFuncCount;
extern const PortDataRun gPortData[];
extern const u32         gPortDataCount;
extern const char        gPortNames[];
}

// ---- GX state recorded by the SDK layer (port_sdk.cpp) until a renderer uses it ----
typedef struct PortGxState {
    f32   viewport[6];        // left, top, width, height, near, far
    s32   scissor[4];         // x, y, width, height
    s32   copySrc[4];         // display copy source: x, y, width, height
    s32   copyDst[2];         // frame buffer width, lines
    f32   copyYScale;
    s32   copyGamma;
    u8    copyAa, copyVf;
    u8    copyPattern[12][2];
    u8    copyVfilter[7];
    u8    clearColor[4];
    u32   clearZ;
    void* lastCopyDst;        // frame buffer of the last GXCopyDisp
    s32   pixelFmt, zFmt;
    u8    dither, colorUpdate;
    u8    zEnable, zFunc, zUpdate;
    u8    vtxDesc[26];        // per attribute: none / direct / index8 / index16
    struct { u8 cnt, type, frac; } vtxFmt[8][26];
    struct { u8 func, src, mtx, normalize, postMtx; } texGen[8];
    u8    numTexGens, numChans, numTevStages;
    u8    lineWidth, pointSize, cullMode, clipMode;
    s32   scissorOffset[2];
    f32   posMtx[10][3][4];
    f32   nrmMtx[10][3][4];   // the upper 3x3 is used
    f32   texMtx[10][3][4];
    f32   postTexMtx[21][3][4];
    u8    currentMtx;
    u8    alphaComp[5];       // comp0, ref0, op, comp1, ref1
    u8    blend[4];           // type, source factor, destination factor, logic op
    u8    alphaUpdate, zCompBeforeTex, dstAlphaOn, dstAlpha;
    s32   fogType;
    u8    fogColor[4];
    f32   fog[4];             // start z, end z, near z, far z
} PortGxState;
extern PortGxState gPortGx;

struct HWND__;
// ---- the player's disc image (port_disc.cpp) ----
enum { PORT_DISC_OK, PORT_DISC_NO_FILE, PORT_DISC_NOT_WII, PORT_DISC_WRONG_GAME, PORT_DISC_WRONG_REGION, PORT_DISC_UNREADABLE };
int  PortDiscCheck(const char* path, char* why, u32 n);    // the header only: is this Monster Lab (RM7P4F)? why says it in words
int  PortDiscOpen(const char* path, char* why, u32 n);     // also the game partition's key and file table
void PortDiscClose(void);
BOOL PortDiscIsOpen(void);
BOOL PortDiscFind(const char* path, u64* offset, u32* size);   // a file of the disc, by its path
BOOL PortDiscRead(u64 offset, void* dst, u32 len);         // decrypted bytes of the game partition
u64  PortDiscDolOffset(void);                              // where main.dol starts in the partition
int  PortSelfTestDisc(void);

// ---- choosing the game disc (port_picker.cpp) ----
int  PortGameCheck(const char* path, char* why, u32 n);    // PORT_DISC_*: a disc image, or a folder with the extracted disc
BOOL PortPickFile(HWND__* owner, char* out, u32 n);        // Windows' file dialog
BOOL PortPickFolder(HWND__* owner, char* out, u32 n);
// The first-run screen; TRUE with the chosen path in out. With shot it is only drawn into that PNG.
BOOL PortPickerRun(const char* note, const char* path, char* out, u32 n, const char* shot);
int  PortSelfTestPicker(void);

// ---- mod tool (port_modtool.cpp, docs/concepts/mod-tools.md): ml_modtool.exe, or --modtool ----
// Runs instead of the game once the disc is open; with shot it draws one picture into that PNG. Exit code.
int  PortModToolRun(const char* open, const char* select, int row, const char* shot);
int  PortModToolTest(void);              // --modtool-test: opens a few files of the player's disc and checks them
int  PortSelfTestModTool(void);

// ---- runtime (port_runtime.cpp) ----
typedef struct PortOptions {
    const char* dataDir;      // extracted disc: <dataDir>/sys/main.dol and <dataDir>/files/; or the disc image itself (PortDiscIsOpen)
    s32  frameLimit;          // stop after this many presented frames (0 = none)
    s32  secondsLimit;        // stop after this many seconds without reaching the frame limit
    BOOL selfTest;
    BOOL verbose;             // log every SDK stub call, not only the first
    BOOL keepGoing;           // a missing function returns 0 instead of stopping (to survey the boot path)
    BOOL haveData;            // main.dol was found and loaded
    BOOL window;              // open a window and draw into it
    BOOL gpu;                 // --gpu (or PORT_GPU=1): draw on the graphics card without a window too (headless runs use WARP)
    BOOL d3dDebug;            // --d3d-debug: Direct3D 11 debug layer; its messages go to the log (needs the Windows Graphics Tools)
    const char* screenshot;   // write the last shown picture to this PNG when the run stops
    const char* pickerShot;   // --picker-screenshot FILE: draw the first-run screen into this PNG and stop (tests)
    const char* dumpData;     // --dump-data DIR: the game's text table as JSON, then exit (port_moddata.cpp)
    const char* dumpTextures; // --dump-textures DIR: every texture the game draws with, as a PNG (port_mods.cpp)
    const char* diffTest;     // --difftest FILE: run the differential tests of that jobs file and stop (port_difftest.cpp)
    BOOL noMods;              // --no-mods: the mods list is kept (and saved) but no mod is used
    BOOL screenshotStats;     // draw the stats overlay into that picture too (--screenshot-stats)
    const char* wav;          // write everything the mixer produces to this WAV file
    BOOL noMovies;            // --no-movies: Bink movies are not played (the game skips them)
    BOOL noAudio;             // never open the sound device
    s32  fastForward;         // run the first N presented frames unpaced (--fast-forward N)
    BOOL fastAll;             // never pace (--fast)
    const char* perfLog;      // --perf-log FILE: one CSV row per shown frame, summary at the end (port_perf.cpp)
    s32  bench;               // --bench menu (1) | village (2) | battle (3) | castle (4): fixed scenario, 1200 frames measured, then exit
    BOOL toMenu;              // --to-menu: fast-forward to the title, press A, go on to the main menu
    s32  showTexture;         // development aid (port_demo.cpp): -1, or a texture of global_binary.sir
    s32  captureAt;           // --capture-at N: take the F12 capture at frame N (tests; 0 = off)
    s32  captureTo;           // --capture-at FROM-TO/N: the last frame of a series of captures,
    s32  captureEvery;        //   and the frames between two of them (0 = one capture)
} PortOptions;
extern PortOptions gPortOptions;

enum { PORT_FPS_UNCAPPED = -1, PORT_FPS_DISPLAY = -2 };
// F12 capture (port_capture.cpp): called by the renderer at each display copy, with the picture just drawn.
void PortCaptureFrame(void);
BOOL PortBinkSaveFrame(const char* path);   // the movie on show, decoded and converted to RGB as a PNG; FALSE when none plays (port_bink.cpp)
BOOL PortGfxCapture(const char* prefix);    // frame buffer, output picture and (once presented) the back buffer as PNGs prefix_*.png
BOOL PortGfxIdle(void);            // the drawing thread waits: show the last picture again if the frame rate asks for it (lock held)
enum { PORT_FILTER_GAME, PORT_FILTER_NEAREST, PORT_FILTER_BILINEAR, PORT_FILTER_AF2, PORT_FILTER_AF4, PORT_FILTER_AF8, PORT_FILTER_AF16, PORT_FILTER_COUNT };
extern const char* const kPortFilterNames[PORT_FILTER_COUNT];   // ini / command-line spelling
void PortGfxApplySettings(void);   // texture filtering changed: textures and samplers are rebuilt at the next picture

// What the player can change (the settings screen edits this; the command line fills it for now).
typedef struct PortSettings {
    s32  width, height;       // output size in pixels (--res WxH, default 1920x1080)
    BOOL wide;                // 16:9 (the console's widescreen setting, SCGetAspectRatio) or 4:3 (--aspect)
    BOOL stretchUi;           // 16:9 only: stretch the 2D layer over the whole width as the console does (--stretch-ui)
    BOOL vsync;               // wait for the monitor at each shown frame
    BOOL fullscreen;          // borderless window over the whole monitor
    s32  volume;              // 0-100, of the sound device's output
    BOOL mute;
    char gamePath[260];       // the player's disc image or extracted folder ([game] path; chosen in the first-run screen)
    s32  renderScale;         // internal frame buffer size in percent of the output picture, 50-200 (--render-scale; applies at start)
    s32  texFilter;           // PORT_FILTER_*: how textures are sampled (--filter)
    s32  fpsCap;              // pictures per second: 0 off (one per game frame), N Hz, PORT_FPS_UNCAPPED, PORT_FPS_DISPLAY (--fps-cap)
    s32  msaa;                // samples of the frame buffer: 0 off, 2, 4 or 8 (--msaa; applies at start)
    BOOL remoteSpeaker;       // play the sounds the console sends to the Wii Remote's speaker through the PC output ([audio] remote_speaker)
    BOOL windowsCursor;       // show the Windows mouse cursor over the picture too ([display] windows_cursor; the game draws its own pointer)
    BOOL dumpTextures;        // save every texture the game draws with as a PNG ([textures] dump; Settings > Mods)
    char dumpDir[260];        // where to; empty: dump\textures next to the exe ([textures] folder)
    s32  lighting;            // PORT_LIGHTING_*: lit 3D draws as GX gives them, or without early clipping and tone-mapped ([graphics] lighting, --lighting)
    s32  exposure;            // improved lighting only: brightness of lit 3D draws before tone mapping, percent 50-150 ([graphics] exposure)
    s32  shadows;             // PORT_SHADOWS_*: the game's hard black shadows, or soft see-through ones ([graphics] shadows, --shadows)
    s32  shadowDark;          // soft shadows only: how much they darken, percent 20-100 ([graphics] shadow_darkness)
} PortSettings;
enum { PORT_LIGHTING_ORIGINAL = 0, PORT_LIGHTING_IMPROVED = 1 };
enum { PORT_SHADOWS_ORIGINAL = 0, PORT_SHADOWS_SOFT = 1 };
extern PortSettings gPortSettings;
// monsterlab.ini next to the exe (port_settings.cpp): display, audio and the input bindings.
const char* PortSettingsPath(void);
BOOL PortSettingsLoad(const char* path, PortSettings* s);        // FALSE: no such file (nothing changed)
BOOL PortSettingsSave(const char* path, const PortSettings* s);
int  PortSelfTestSettings(void);

void PortLog(const char* fmt, ...);
// Records the first use of an SDK feature (shown in the boot summary); returns TRUE the first time.
BOOL PortMilestone(const char* what);
// Counts a call to an SDK function that does nothing on PC yet; logs the first call.
void PortStubCall(const char* name);
// SDK functions without a PC implementation that may return 0 for now (port_sdk.cpp): the text
// to log, or NULL if a call to this address must stop the run.
const char* PortSoftSdk(u32 wiiAddr);
// port_files.cpp: called for every completed disc read; converts big-endian .sir sections in place.
void PortFileRead(const char* path, u32 offset, u8* dst, u32 len);
// TRUE when p lies in the image of a .sir section (data still in console byte order unless the loader converts it).
BOOL PortIsSectionData(const void* p);
const char* PortFileStats(char* buf, u32 n);
// zlib streams (port_zlib.cpp): a stream that starts on the bytes of a packed .sir read is bound to its
// section; when it ends the section is converted. Drop forgets a stream (re-init, end).
void PortFileInflateBegin(void* strm, const u8* nextIn, u8* nextOut);
void PortFileInflateEnd(void* strm, u32 totalOut);
void PortFileInflateDrop(void* strm);
int  PortSelfTestZlib(void);
void PortDemoMain(void);              // port_demo.cpp
// Prints the boot summary and ends the process.
void PortStop(int exitCode, const char* fmt, ...);
void PortAudioTick(void);     // port_audio.cpp: run the AX frames that are due (interrupt thread)
const char* PortAudioStats(char* buf, u32 n);
void PortAudioMoviePush(const float* stereo, u32 frames, int rate);   // a Bink movie's sound (port_bink.cpp)
void PortAudioMovieStop(void);
void PortAudioMovieVolume(float v);
u32  PortAudioVoices(u32* outOf); // voices the mixer is playing now
extern s64 gPortWaitTicks;    // QueryPerformanceCounter ticks spent asleep in VIWaitForRetrace (port_sdk.cpp)
int  PortSelfTestAudio(void);
void PortFrameDone(void);     // one presented frame (VI flip): enforces the frame limit
s32  PortFrameCount(void);    // presented frames so far

const PortFunc* PortFindFunc(u32 wiiAddr);             // exact start address
const char*     PortFuncName(const PortFunc* f);
// Describes a PC code address as "Name+0xNN" (nearest linked function); buf is returned.
const char*     PortDescribePc(const void* pc, char* buf, u32 bufSize);
BOOL            PortIsWiiAddr(u32 addr);
u32             PortArenaLoMEM1(void);                 // first free byte after the executable's data

// ---- the big lock (port_runtime.cpp) ----
// The Wii has one core: game threads never run at the same time and interrupts arrive between
// instructions. On PC every game thread, and the thread that delivers "interrupts" (video retrace,
// disc read completion, alarms), runs only while it holds this lock and gives it up only where it
// would block or poll.
void PortLockEnter(void);
void PortLockLeave(void);
void PortYield(void);                 // let waiting threads and interrupts run, if any
void PortWaitChange(void);            // leave the lock until another thread calls PortSignalChange
void PortSignalChange(void);
u32  PortDisableInterrupts(void);     // returns the previous state (1 = were enabled)
void PortRestoreInterrupts(u32 state);
// Runs fn(arg) on the interrupt thread, with the lock held, at the next opportunity.
void PortQueueInterrupt(void (*fn)(void* arg), void* arg);

// ---- hooks implemented by the SDK layer ----
void PortOsStart(void);               // heaps, time base (port_os.cpp)
void PortOsTick(void);                // alarms, called from the interrupt thread about every ms
void PortViTick(void);                // video retrace (port_sdk.cpp)
s64    PortViNextTicks(void);         // when the next retrace is due (0 = not started)
s64    PortAudioNextTicks(void);      // when the next AX frame is due (0 = audio not running)
s64    PortOsNextAlarm(void);         // the earliest alarm (0 = none)
int  PortSelfTestSdk(void);           // 0 = ok

// ---- the clock (port_os.cpp) ----
// Game time is a virtual clock in OSTime ticks. Paced, it follows the real clock. Fast-forwarding
// (--fast-forward N, --fast), it stands still while game code runs and jumps to the next event
// (retrace, AX frame, alarm, sleep deadline) when every game thread waits; the switch back to the
// real clock continues from the same time. Retraces, alarms, OSSleep and the audio tick all use it.
s64    PortTicks(void);               // the game clock, without side effects
BOOL   PortFastActive(void);          // TRUE while fast-forwarding
void   PortFastEnd(void);             // back to real-time pacing (continues from the same time)
void   PortClockWakeAt(s64 t);     // signal the waiters when the clock reaches t (OSSleep)
BOOL   PortClockAdvanceIfIdle(void);  // interrupt thread: all game threads wait, so jump to the next event
void   PortGameThreads(s32 delta);    // game threads that exist (the main thread counts)
const char* PortDvdStats(char* buf, u32 bufSize);
