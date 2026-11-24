// PC audio: the SDK's AX voices, the MIX and AXART libraries on top of them, a software mixer
// and a Windows wave output. No original code; what each function has to do was read from the
// SDK code in main.dol (docs/sdk_libraries.md).
//
// The game plays everything on AX voices: sound effects are .dsp records (DSP-ADPCM, PCM16 or
// PCM8) held in memory, music is two streamed .dsp files (left, right) whose ring buffers the
// game refills from the disc while it watches each voice's current address. Its audio engine
// runs in the callback AXRegisterCallback takes, once per AX frame (3 ms, 96 samples at 32 kHz).
//
// Here PortAudioTick (interrupt thread, lock held) runs the AX frames the clock says are due:
// the game's callback, then the mixer. The samples go to the wave device (only with --window),
// to --wav FILE, or nowhere (headless runs, CI).
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mutex>
#include <deque>

#include "port/port.h"

#define AX_VOICES        64
#define AX_FRAME         96          // samples per AX frame
#define AX_RATE          32000
#define VOICE_SIZE       0x200       // AXVPB: 0x28 bytes of list and callback, then the parameter block

// AXVPB offsets (the game reads the state and the current address itself)
enum {
    V_PRIORITY = 0x0C, V_CALLBACK = 0x10, V_CONTEXT = 0x14, V_INDEX = 0x18, V_SYNC = 0x1C,
    V_SRCSEL = 0x30, V_COEFSEL = 0x32, V_STATE = 0x38, V_TYPE = 0x3A, V_MIX = 0x3C,
    V_VE = 0x92, V_LOOPFLAG = 0x96, V_FORMAT = 0x98, V_LOOPADDR = 0x9A, V_ENDADDR = 0x9E, V_CURADDR = 0xA2,
    V_COEF = 0xA6, V_GAIN = 0xC6, V_PS = 0xC8, V_YN1 = 0xCA, V_YN2 = 0xCC,
    V_RATIO = 0xCE, V_LOOP_PS = 0xDC, V_LOOP_YN1 = 0xDE, V_LOOP_YN2 = 0xE0, V_RMT = 0xFE,
};

static u8 sVoices[AX_VOICES][VOICE_SIZE];
struct VoiceRun { BOOL used, running; u32 frac; s32 s0, s1; s32 volL, volR; };
static VoiceRun sRun[AX_VOICES];
static void (*sAxCallback)(void);
static BOOL sAxInit;
static s32  sAxMode;
static struct { u32 frames, started, samplesOut, peak, dropped; } sStats;

static inline u16& H(u8* v, u32 off) { return *(u16*)(v + off); }
static inline u32  Get32(u8* v, u32 off) { return ((u32)H(v, off) << 16) | H(v, off + 2); }
static inline void Set32(u8* v, u32 off, u32 x) { H(v, off) = (u16)(x >> 16); H(v, off + 2) = (u16)x; }
static inline int  Index(u8* v) { return (int)((v - sVoices[0]) / VOICE_SIZE); }
// A DSP address is a physical address in the sample format's units; main memory is mapped at
// its console addresses.
static inline u8*  Phys(u32 byteAddr) { return (u8*)(uintptr_t)(byteAddr | 0x80000000u); }

extern "C" {

// ------------------------------------------------------------------ AX
void AIInit(u8* stack) { PortMilestone("AIInit"); }

void AXInit(void) {
    if (sAxInit) return;
    PortMilestone("AXInit");
    memset(sVoices, 0, sizeof(sVoices));
    memset(sRun, 0, sizeof(sRun));
    for (int i = 0; i < AX_VOICES; i++) *(s32*)(sVoices[i] + V_INDEX) = i;
    sAxInit = TRUE;
}

void AXQuit(void) { sAxInit = FALSE; sAxCallback = NULL; }

// AXRegisterCallback: the function to call once per AX frame; returns the one before.
void* AXRegisterCallback(void* cb) {
    void* old = (void*)sAxCallback;
    sAxCallback = (void (*)(void))cb;
    return old;
}

// AXAcquireVoice(priority 1..31, callback(voice) for when the voice is taken away, context):
// a free voice, else the lowest-priority voice below `priority` (its owner is told), else NULL.
s32 AXAcquireVoice(s32 priority, void* cb, void* ctx) {
    int pick = -1;
    for (int i = 0; i < AX_VOICES; i++) if (!sRun[i].used) { pick = i; break; }
    if (pick < 0) {
        s32 low = priority;
        for (int i = 0; i < AX_VOICES; i++) {
            s32 p = *(s32*)(sVoices[i] + V_PRIORITY);
            if (p < low) { low = p; pick = i; }
        }
        if (pick < 0) return 0;
        void (*drop)(u8*) = *(void (**)(u8*))(sVoices[pick] + V_CALLBACK);
        if (drop) drop(sVoices[pick]);
    }
    u8* v = sVoices[pick];
    memset(v, 0, VOICE_SIZE);
    *(s32*)(v + V_INDEX) = pick;
    *(s32*)(v + V_PRIORITY) = priority;
    *(void**)(v + V_CALLBACK) = cb;
    *(void**)(v + V_CONTEXT) = ctx;
    H(v, V_VE) = 0x8000;
    H(v, V_SRCSEL) = 1;
    memset(&sRun[pick], 0, sizeof(sRun[pick]));
    sRun[pick].used = TRUE;
    return (s32)(uintptr_t)v;
}

void AXFreeVoice(u8* v) {
    if (!v) return;
    H(v, V_STATE) = 0;
    *(s32*)(v + V_PRIORITY) = 0;
    sRun[Index(v)].used = FALSE;
    sRun[Index(v)].running = FALSE;
}

void AXSetVoicePriority(u8* v, u32 priority) { *(u32*)(v + V_PRIORITY) = priority; }

// The aux bus effects (AXFX reverb, delay, chorus) are not mixed on PC: the callbacks are taken
// and never called.
void AXRegisterAuxACallback(void* cb, void* ctx) {}
void AXRegisterAuxBCallback(void* cb, void* ctx) {}
s32  AXSetMode(s32 mode) { sAxMode = mode; return 0; }
// Two getters of the DSP's cycle budget; the game ignores what they return.
void fn_8025B390(void) {}
void fn_8025B398(void) {}
// The Wii Remote's speaker: no samples to send.
s32  AXRmtGetSamples(s32 chan, u8* buf, s32 num) { return 0; }
s32  AXRmtAdvancePtr(s32 num) { return 0; }

// AXSetVoiceSrcType: 0 none, 1 linear, 2..4 four-tap filters. The mixer always interpolates linearly.
s32 AXSetVoiceSrcType(u8* v, s32 type) {
    switch (type) {
    case 0: H(v, V_SRCSEL) = 2; break;
    case 1: H(v, V_SRCSEL) = 1; break;
    case 2: H(v, V_SRCSEL) = 0; H(v, V_COEFSEL) = 0; break;
    case 3: H(v, V_SRCSEL) = 0; H(v, V_COEFSEL) = 1; break;
    case 4: H(v, V_SRCSEL) = 0; H(v, V_COEFSEL) = 2; break;
    }
    *(u32*)(v + V_SYNC) |= 0x1;
    return 0;
}
void AXSetVoiceState(u8* v, s32 state) {
    if (H(v, V_STATE) == (u16)state) return;
    H(v, V_STATE) = (u16)state;
    *(u32*)(v + V_SYNC) |= 0x4;
}
void AXSetVoiceType(u8* v, s32 type) { H(v, V_TYPE) = (u16)type; *(u32*)(v + V_SYNC) |= 0x8; }
// AXSetVoiceMix: 12 pairs of volume (0x8000 = full) and step; the first two are left and right.
s32 AXSetVoiceMix(u8* v, u16* mix) { memcpy(v + V_MIX, mix, 0x30); *(u32*)(v + V_SYNC) |= 0x12; return 0; }
// AXSetVoiceAddr: loop flag, format (0 ADPCM, 0xA PCM16, 0x19 PCM8), loop / end / current address.
s32 AXSetVoiceAddr(u8* v, u16* a) {
    memcpy(v + V_LOOPFLAG, a, 0x10);
    if (a[1] == 0x0A) { memset(v + V_COEF, 0, 0x28); H(v, V_GAIN) = 0x0800; }
    else if (a[1] == 0x19) { memset(v + V_COEF, 0, 0x28); H(v, V_GAIN) = 0x0100; }
    *(u32*)(v + V_SYNC) = (*(u32*)(v + V_SYNC) & ~0x7800u) | 0x8400;
    return 0;
}
// AXSetVoiceAdpcm: 16 coefficients, gain, predictor/scale, two history samples.
s32 AXSetVoiceAdpcm(u8* v, u16* a) { memcpy(v + V_COEF, a, 0x28); *(u32*)(v + V_SYNC) |= 0x8000; return 0; }
// AXSetVoiceSrc: pitch ratio (16.16, source rate / 32000), then the resampler's state.
s32 AXSetVoiceSrc(u8* v, u16* s) { memcpy(v + V_RATIO, s, 0x0E); *(u32*)(v + V_SYNC) = (*(u32*)(v + V_SYNC) & ~0x20000u) | 0x10000; return 0; }
s32 AXSetVoiceAdpcmLoop(u8* v, u16* l) { memcpy(v + V_LOOP_PS, l, 6); *(u32*)(v + V_SYNC) |= 0x40000; return 0; }
s32 AXSetVoiceRmtOn(u8* v, s32 on) { H(v, V_RMT) = (u16)on; *(u32*)(v + V_SYNC) |= 0x800000; return 0; }

static void SetVoiceSrcRatio(u8* v, f32 ratio) {
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 4.0f) ratio = 4.0f;
    Set32(v, V_RATIO, (u32)(ratio * 65536.0f));
    *(u32*)(v + V_SYNC) |= 0x20000;
}

// ------------------------------------------------------------------ MIX
// One channel per AX voice: levels in tenths of a decibel (0 = full, -904 and below = silent),
// pan 0 (left) .. 127 (right), span (front/back, not used with two speakers), fader.
struct MixChan { u8* voice; BOOL mute; s32 input, aux[3], pan, span, fader; };
static MixChan sMix[AX_VOICES];
static s32 sMixMode;

static s32 Level(s32 tenths) {
    if (tenths <= -904) return 0;
    if (tenths > 60) tenths = 60;
    s32 v = (s32)(32768.0 * pow(10.0, tenths / 200.0) + 0.5);
    return v > 0xFFFF ? 0xFFFF : v;
}
// Left attenuation for a pan position, in tenths of a decibel; -3 dB in the middle.
static s32 PanTenths(s32 pan) {
    if (pan <= 0) return 0;
    if (pan >= 127) return -904;
    return (s32)(200.0 * log10(cos(pan * 1.5707963 / 128.0)));
}
static void MixApply(MixChan* c) {
    u8* v = c->voice;
    if (!v) return;
    s32 base = c->input + c->fader;
    u16 l = 0, r = 0;
    if (!c->mute) {
        if (sMixMode == 0) l = r = (u16)Level(base - 30);                 // mono
        else { l = (u16)Level(base + PanTenths(c->pan)); r = (u16)Level(base + PanTenths(c->pan <= 0 ? 127 : 128 - c->pan)); }
    }
    H(v, V_MIX + 0) = l;
    H(v, V_MIX + 4) = r;
    *(u32*)(v + V_SYNC) |= 0x10;
}
static MixChan* Chan(u8* v) { return v ? &sMix[Index(v)] : NULL; }

s32  MIXInit(void) { memset(sMix, 0, sizeof(sMix)); return 0; }
void MIXQuit(void) { memset(sMix, 0, sizeof(sMix)); }
// MIXSetSoundMode: 0 mono, 1 stereo, 2 surround, 3 Dolby Pro Logic 2 (mixed as stereo here).
s32  MIXSetSoundMode(s32 mode) { sMixMode = mode; return 0; }
void MIXInitChannel(u8* v, s32 mode, s32 input, s32 auxA, s32 auxB, s32 auxC, s32 pan, s32 span, s32 fader) {
    MixChan* c = Chan(v);
    if (!c) return;
    c->voice = v; c->mute = (mode & 8) != 0;
    c->input = input; c->aux[0] = auxA; c->aux[1] = auxB; c->aux[2] = auxC;
    c->pan = pan; c->span = span; c->fader = fader;
    MixApply(c);
}
void MIXReleaseChannel(u8* v) { if (v) sMix[Index(v)].voice = NULL; }
void MIXMute(u8* v) { if (v) sMix[Index(v)].mute = TRUE; }
void MIXUnMute(u8* v) { if (v) sMix[Index(v)].mute = FALSE; }
s32  MIXIsMute(u8* v) { return v ? sMix[Index(v)].mute : 0; }
void MIXSetFader(s32 voice, s32 fader) { if (voice) sMix[Index((u8*)(uintptr_t)voice)].fader = fader; }
// MIXUpdateSettings: once per AX frame, the channels' levels become the voices' mix.
void MIXUpdateSettings(void) { for (int i = 0; i < AX_VOICES; i++) MixApply(&sMix[i]); }

// ------------------------------------------------------------------ AXART
// A sound: +0x00 next, +0x04 previous, +0x08 the AX voice, +0x0C f32 sample rate, +0x10 its list
// of articulators (+0x00 next, +0x04 type). Every AX frame the articulators are summed up into
// the voice's pitch, level, pan and fader. Types the game uses: 1 position in 3D, 2 pan, 5 pitch
// (cents, 16.16), 8 volume and 9..11 aux sends (tenths of a decibel, 16.16), 20 low-pass,
// 22 and 24 the remote's speaker. The envelope and modulation types (6, 7, 12..19) are not used.
static u8* sArtSounds;
static f32 sArtDoppler = 20.0f, sArtDistance = 40.0f;

// The 3D articulator: +0x08 horizontal angle, +0x0C vertical angle (radians), +0x10 distance,
// +0x14 closing speed, +0x18 "changed". The SDK turns the angles into pan, span, delay and filter
// through tables of 360 entries (lbl_8039EF30: pan, +0x168 span, +0x2D0 pan for Pro Logic 2,
// +0x438 / +0x5A0 delay left / right, +0x708 filter by the vertical angle).
static u32 AngleIndex(f32 a) {
    if (a > 0.0f) return (u32)(360.0f * (a / 6.283185f)) % 360;
    if (a < 0.0f) { f32 d = 360.0f + 360.0f * (a / 6.283185f); return d > 0.0f ? (u32)d % 360 : 0; }
    return 0;
}
static void Art3dUpdate(u8* a) {
    if (*(s32*)(a + 0x18) == 0) return;
    const u8* tab = (const u8*)(uintptr_t)0x8039EF30;
    u32 h = AngleIndex(*(f32*)(a + 0x08)), vert = AngleIndex(*(f32*)(a + 0x0C));
    *(s32*)(a + 0x18) = 0;
    a[0x1C] = (sMixMode == 3) ? tab[0x2D0 + h] : tab[h];
    a[0x1D] = tab[0x168 + h];
    *(u16*)(a + 0x20) = tab[0x438 + h];
    *(u16*)(a + 0x22) = tab[0x5A0 + h];
    a[0x1E] = tab[0x708 + vert];
    *(f32*)(a + 0x24) = *(f32*)(a + 0x14) / sArtDoppler;
    *(s32*)(a + 0x28) = (s32)(-3932160.0f * (*(f32*)(a + 0x10) / sArtDistance));
}

static void ArtEvaluate(u8* s, BOOL first) {
    u8* v = *(u8**)(s + 0x08);
    f32 ratio = *(f32*)(s + 0x0C) / 32000.0f;
    s32 cents = 0, volume = 0, aux[3] = {0, 0, 0}, fader = 0;
    s32 pan = 0x40, span = 0x7F, src = 1;
    u16 rmt = 0;
    for (u8* a = *(u8**)(s + 0x10); a; a = *(u8**)a) {
        switch (*(u32*)(a + 4)) {
        case 1:
            Art3dUpdate(a);
            ratio += *(f32*)(a + 0x24);
            volume += *(s32*)(a + 0x28);
            pan = a[0x1C]; span = a[0x1D]; src = a[0x1E];
            break;
        case 2:  pan = a[8]; span = a[9]; break;
        case 4:  src = a[8]; break;
        case 5:  cents += *(s32*)(a + 8); break;
        case 6:  cents += *(s32*)(a + 0x10); break;
        case 8:  volume += *(s32*)(a + 8); break;
        case 9:  aux[0] += *(s32*)(a + 8); break;
        case 10: aux[1] += *(s32*)(a + 8); break;
        case 11: aux[2] += *(s32*)(a + 8); break;
        case 12: volume += *(s32*)(a + 0x10); break;
        case 21: fader = *(s32*)(a + 8); break;
        case 22: rmt = *(u16*)(a + 8); break;
        }
    }
    if (!v) return;
    AXSetVoiceSrcType(v, src);
    SetVoiceSrcRatio(v, ratio * (f32)pow(2.0, (cents >> 16) / 1200.0));
    if (first) {
        MIXInitChannel(v, 0, volume >> 16, aux[0] >> 16, aux[1] >> 16, aux[2] >> 16, pan, span, fader >> 16);
    } else {
        MixChan* c = Chan(v);
        c->input = volume >> 16;
        c->aux[0] = aux[0] >> 16; c->aux[1] = aux[1] >> 16; c->aux[2] = aux[2] >> 16;
        c->pan = pan; c->span = span; c->fader = fader >> 16;
    }
    AXSetVoiceRmtOn(v, rmt);
}

s32  AXARTInit(void) { sArtSounds = NULL; sArtDoppler = 20.0f; sArtDistance = 40.0f; return 0; }
void AXARTQuit(void) {
    for (u8* s = sArtSounds; s; s = *(u8**)s) MIXReleaseChannel(*(u8**)(s + 8));
    sArtSounds = NULL;
}
void AXARTServiceSounds(void) { for (u8* s = sArtSounds; s; s = *(u8**)s) ArtEvaluate(s, FALSE); }
s32  AXARTInitSound(u8* s, s32 voice, s32 rate) {
    *(u8**)(s + 0x10) = NULL;
    *(s32*)(s + 0x08) = voice;
    *(f32*)(s + 0x0C) = (f32)(u32)rate;
    return 0;
}
s32 AXARTAddSound(u8* s) {
    ArtEvaluate(s, TRUE);
    *(u8**)(s + 0) = sArtSounds;
    *(u8**)(s + 4) = NULL;
    if (sArtSounds) *(u8**)(sArtSounds + 4) = s;
    sArtSounds = s;
    return 0;
}
void AXARTRemoveSound(u8* s) {
    u8* next = *(u8**)(s + 0); u8* prev = *(u8**)(s + 4);
    if (prev) *(u8**)(prev + 0) = next; else if (sArtSounds == s) sArtSounds = next;
    if (next) *(u8**)(next + 4) = prev;
    MIXReleaseChannel(*(u8**)(s + 8));
}
s32 AXARTAddArticulator(u8* s, u8* a) { *(u8**)a = *(u8**)(s + 0x10); *(u8**)(s + 0x10) = a; return 0; }
s32 AXARTRemoveArticulator(u8* s, u8* a) {
    u8** link = (u8**)(s + 0x10);
    for (; *link; link = (u8**)*link) if (*link == a) { *link = *(u8**)a; return 1; }
    return 0;
}
s32 AXARTSet3DDopplerScale(f32 scale) { sArtDoppler = scale; return 0; }
s32 AXARTSet3DDistanceScale(f32 scale) { sArtDistance = scale; return 0; }

void AXARTInitArt3D(u8* a) {
    *(u32*)(a + 4) = 1;
    *(f32*)(a + 0x08) = *(f32*)(a + 0x0C) = *(f32*)(a + 0x10) = *(f32*)(a + 0x14) = 0.0f;
    *(s32*)(a + 0x18) = 0;
    a[0x1C] = 0x40; a[0x1D] = 0x7F; a[0x1E] = 1;
    *(u16*)(a + 0x20) = 0; *(u16*)(a + 0x22) = 0;
    *(f32*)(a + 0x24) = 0.0f;
    *(s32*)(a + 0x28) = (s32)0xFC400000;
}
s32  AXARTInitArtPanning(u8* a) { *(u32*)(a + 4) = 2; a[8] = 0x40; a[9] = 0x7F; return 0; }
s32  AXARTInitArtPitch(u8* a) { *(u32*)(a + 4) = 5; *(s32*)(a + 8) = 0; return 0; }
s32  AXARTInitArtVolume(u8* a) { *(u32*)(a + 4) = 8; *(s32*)(a + 8) = 0; return 0; }
s32  AXARTInitArtAuxAVolume(u8* a) { *(u32*)(a + 4) = 9; *(s32*)(a + 8) = 0; return 0; }
s32  AXARTInitArtAuxBVolume(u8* a) { *(u32*)(a + 4) = 10; *(s32*)(a + 8) = 0; return 0; }
s32  AXARTInitArtAuxCVolume(u8* a) { *(u32*)(a + 4) = 11; *(s32*)(a + 8) = 0; return 0; }
void AXARTInitArtLpf(u8* a) { *(u32*)(a + 4) = 0x14; *(s32*)(a + 8) = 1; *(s32*)(a + 0xC) = 0; *(s32*)(a + 0x10) = 1; }
void AXARTInitArtRmt(u8* a) { *(u32*)(a + 4) = 0x16; *(u16*)(a + 8) = 1; }
void AXARTInitArtRmtAuxVolume(u8* a) { *(u32*)(a + 4) = 0x18; memset(a + 8, 0, 0x10); }

}  // extern "C"

// ------------------------------------------------------------------ the mixer
struct Decoder {
    u8* v; u32 addr, loop, end; u16 format, ps; s32 yn1, yn2; BOOL stopped;
};

static void DecoderLoad(Decoder* d, u8* v) {
    d->v = v;
    d->addr = Get32(v, V_CURADDR); d->loop = Get32(v, V_LOOPADDR); d->end = Get32(v, V_ENDADDR);
    d->format = H(v, V_FORMAT);
    d->ps = H(v, V_PS); d->yn1 = (s16)H(v, V_YN1); d->yn2 = (s16)H(v, V_YN2);
    d->stopped = FALSE;
}
static void DecoderStore(Decoder* d) {
    u8* v = d->v;
    Set32(v, V_CURADDR, d->addr);
    H(v, V_PS) = d->ps; H(v, V_YN1) = (u16)d->yn1; H(v, V_YN2) = (u16)d->yn2;
    if (d->stopped) H(v, V_STATE) = 0;
}
// The next sample of the voice. At the end address a looping voice goes to its loop address
// (an ADPCM voice of type 0 also takes the loop's predictor and history; a stream keeps its
// history); any other voice stops there.
static s32 DecoderNext(Decoder* d) {
    if (d->stopped) return 0;
    u8* v = d->v;
    s32 s;
    if (d->format == 0) {
        if ((d->addr & 0xF) == 0) { d->ps = *Phys(d->addr >> 1); d->addr += 2; }
        u8 byte = *Phys(d->addr >> 1);
        s32 nib = (d->addr & 1) ? (byte & 0xF) : (byte >> 4);
        if (nib >= 8) nib -= 16;
        u32 idx = (d->ps >> 4) & 7;
        s32 c1 = (s16)H(v, V_COEF + idx * 4), c2 = (s16)H(v, V_COEF + idx * 4 + 2);
        s = ((nib << (d->ps & 0xF)) << 11) + c1 * d->yn1 + c2 * d->yn2;
        s = (s + 1024) >> 11;
        s = s < -32768 ? -32768 : s > 32767 ? 32767 : s;
        d->yn2 = d->yn1; d->yn1 = s;
    } else if (d->format == 0x0A) {
        const u8* p = Phys(d->addr * 2);
        s = (s16)((p[0] << 8) | p[1]);                    // the samples stay in console byte order
    } else {
        s = (s32)(s8)*Phys(d->addr) << 8;
    }
    if (d->addr == d->end) {
        if (H(v, V_LOOPFLAG)) {
            d->addr = d->loop;
            if (d->format == 0) {
                if ((d->loop & 0xF) == 2) d->ps = *Phys((d->loop - 2) >> 1); else d->ps = H(v, V_LOOP_PS);
                if (H(v, V_TYPE) == 0) { d->yn1 = (s16)H(v, V_LOOP_YN1); d->yn2 = (s16)H(v, V_LOOP_YN2); }
            }
        } else {
            d->addr = d->loop;
            d->stopped = TRUE;
        }
    } else {
        d->addr++;
    }
    return s;
}

// ------------------------------------------------------------------ movie sound
// The decoded sound of a Bink movie (port_bink.cpp): interleaved stereo float samples at the movie's
// rate, mixed into every AX frame on top of the voices. Underrun is silence.
static std::mutex         sMovLock;
static std::deque<float>  sMovQ;                 // left, right, left, right...
static int                sMovRate = 44100;
static float              sMovVol = 1.0f;
static double             sMovFrac = 0;
static float              sMovS0[2], sMovS1[2];

void PortAudioMoviePush(const float* st, u32 frames, int rate) {
    std::lock_guard<std::mutex> g(sMovLock);
    sMovRate = rate;
    sMovQ.insert(sMovQ.end(), st, st + (size_t)frames * 2);
}
void PortAudioMovieStop() {
    std::lock_guard<std::mutex> g(sMovLock);
    sMovQ.clear();
    sMovFrac = 0;
}
void PortAudioMovieVolume(float v) { sMovVol = v; }

static void MixMovie(s32* acc) {
    std::lock_guard<std::mutex> g(sMovLock);
    if (sMovQ.empty() && sMovFrac < 1.0) return;
    double step = (double)sMovRate / AX_RATE;
    for (int n = 0; n < AX_FRAME; n++) {
        while (sMovFrac >= 1.0) {
            if (sMovQ.size() < 2) { sMovFrac = 1.0; return; }
            sMovS0[0] = sMovS1[0]; sMovS0[1] = sMovS1[1];
            sMovS1[0] = sMovQ[0]; sMovS1[1] = sMovQ[1];
            sMovQ.pop_front(); sMovQ.pop_front();
            sMovFrac -= 1.0;
        }
        for (int c = 0; c < 2; c++) {
            float v = sMovS0[c] + (float)((sMovS1[c] - sMovS0[c]) * sMovFrac);
            acc[n * 2 + c] += (s32)(v * 32767.0f * sMovVol);
        }
        sMovFrac += step;
    }
}

// Mixes one AX frame of every running voice into out (interleaved left, right).
static void MixFrame(s16* out) {
    s32 acc[AX_FRAME * 2];
    memset(acc, 0, sizeof(acc));
    for (int i = 0; i < AX_VOICES; i++) {
        u8* v = sVoices[i];
        VoiceRun* r = &sRun[i];
        u32 sync = *(u32*)(v + V_SYNC);
        *(u32*)(v + V_SYNC) = 0;
        if (!r->used || H(v, V_STATE) != 1) { r->running = FALSE; continue; }
        s32 volL = H(v, V_MIX + 0), volR = H(v, V_MIX + 4);
        if (!r->running) {
            r->running = TRUE; r->frac = 0x10000; r->s0 = r->s1 = 0; r->volL = volL; r->volR = volR;
            sStats.started++;
            if (gPortOptions.verbose || getenv("PORT_AUDIO_TRACE")) PortLog("audio: voice %d starts: format %u, type %u, ratio %08X, at %08X, loop %08X (%u), end %08X, level %d / %d", i, H(v, V_FORMAT), H(v, V_TYPE), Get32(v, V_RATIO), Get32(v, V_CURADDR), Get32(v, V_LOOPADDR), H(v, V_LOOPFLAG), Get32(v, V_ENDADDR), volL, volR);
        }
        u32 ratio = Get32(v, V_RATIO);
        Decoder d;
        DecoderLoad(&d, v);
        // The game moved the voice (a stream starting over): the block's own predictor applies.
        if ((sync & 0x400) && d.format == 0 && (d.addr & 0xF) == 2) d.ps = *Phys((d.addr - 2) >> 1);
        s32 stepL = (volL - r->volL) / AX_FRAME, stepR = (volR - r->volR) / AX_FRAME;
        s32 l = r->volL, rr = r->volR;
        for (int n = 0; n < AX_FRAME; n++) {
            while (r->frac >= 0x10000) { r->s0 = r->s1; r->s1 = DecoderNext(&d); r->frac -= 0x10000; }
            s32 s = r->s0 + (s32)(((s64)(r->s1 - r->s0) * (s32)r->frac) >> 16);
            acc[n * 2 + 0] += (s * l) >> 15;
            acc[n * 2 + 1] += (s * rr) >> 15;
            r->frac += ratio;
            l += stepL; rr += stepR;
        }
        r->volL = volL; r->volR = volR;
        DecoderStore(&d);
        if (d.stopped) r->running = FALSE;
    }
    MixMovie(acc);
    for (int n = 0; n < AX_FRAME * 2; n++) {
        s32 s = acc[n];
        s = s < -32768 ? -32768 : s > 32767 ? 32767 : s;
        out[n] = (s16)s;
        u32 mag = (u32)(s < 0 ? -s : s);
        if (mag > sStats.peak) sStats.peak = mag;
    }
}

// ------------------------------------------------------------------ output
// The mixer (interrupt thread, under the big lock) only writes into a ring; a thread of its own
// feeds the wave device from it, so a slow device call never holds up the game.
#define OUT_BLOCKS       8
#define OUT_BLOCK_FRAMES 4                               // AX frames per block handed to the device (12 ms)
#define RING_SAMPLES     32768                           // about a second, a power of two
#define RING_TARGET      (AX_RATE * 6 / 100)             // the mixer runs 60 ms ahead of the clock
static HWAVEOUT sWave;
static WAVEHDR  sHdr[OUT_BLOCKS];
static s16      sBlock[OUT_BLOCKS][OUT_BLOCK_FRAMES * AX_FRAME * 2];
static s16      sRing[RING_SAMPLES * 2];
static volatile LONG sRingWrite, sRingRead;              // in samples, never wrapped: index = value % RING_SAMPLES
static FILE*    sWav;
static u32      sWavSamples;
static s64      sClockStart;                             // game clock (port.h) when the audio started
static u64      sFramesDone;                             // AX frames since sClockStart
static BOOL     sStarted;

static void WavHeader(void) {
    u32 bytes = sWavSamples * 4;
    u8 h[44] = {'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,2,0,0,0,0,0,0,0,0,0,4,0,16,0,'d','a','t','a',0,0,0,0};
    u32 riff = bytes + 36, rate = AX_RATE, bps = AX_RATE * 4;
    memcpy(h + 4, &riff, 4); memcpy(h + 24, &rate, 4); memcpy(h + 28, &bps, 4); memcpy(h + 40, &bytes, 4);
    fseek(sWav, 0, SEEK_SET);
    fwrite(h, 1, 44, sWav);
    fseek(sWav, 0, SEEK_END);
    fflush(sWav);
}

static DWORD WINAPI OutputThread(void*) {
    WAVEFORMATEX f = {};
    f.wFormatTag = WAVE_FORMAT_PCM; f.nChannels = 2; f.nSamplesPerSec = AX_RATE; f.wBitsPerSample = 16;
    f.nBlockAlign = 4; f.nAvgBytesPerSec = AX_RATE * 4;
    HWAVEOUT wave = NULL;
    if (waveOutOpen(&wave, WAVE_MAPPER, &f, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
        PortLog("audio: no sound device, running silent");
        return 0;
    }
    for (int i = 0; i < OUT_BLOCKS; i++) {
        sHdr[i].lpData = (LPSTR)sBlock[i];
        sHdr[i].dwBufferLength = sizeof(sBlock[i]);
        waveOutPrepareHeader(wave, &sHdr[i], sizeof(WAVEHDR));
        sHdr[i].dwFlags |= WHDR_DONE;
    }
    PortLog("audio: wave output, %d Hz stereo", AX_RATE);
    sRingRead = sRingWrite;
    sWave = wave;
    const LONG block = OUT_BLOCK_FRAMES * AX_FRAME;
    for (int cur = 0;;) {
        if (!(sHdr[cur].dwFlags & WHDR_DONE) || sRingWrite - sRingRead < block) { Sleep(2); continue; }
        LONG rd = sRingRead;
        for (LONG n = 0; n < block; n++) {
            const s16* p = &sRing[((u32)(rd + n) % RING_SAMPLES) * 2];
            sBlock[cur][n * 2] = p[0]; sBlock[cur][n * 2 + 1] = p[1];
        }
        sRingRead = rd + block;
        sHdr[cur].dwFlags &= ~WHDR_DONE;
        waveOutWrite(wave, &sHdr[cur], sizeof(WAVEHDR));
        cur = (cur + 1) % OUT_BLOCKS;
    }
}

static void Start(void) {
    sStarted = TRUE;
    sClockStart = PortTicks();
    if (gPortOptions.wav) {
        sWav = fopen(gPortOptions.wav, "wb");
        if (sWav) WavHeader(); else PortLog("audio: cannot write %s", gPortOptions.wav);
    }
    // Headless runs (and CI) never touch a sound device.
    if (gPortOptions.window && !gPortOptions.noAudio) CloseHandle(CreateThread(NULL, 0, OutputThread, NULL, 0, NULL));
}

// One AX frame (3 ms) in game clock ticks.
static s64 FrameTicks(void) { return 60750000LL * AX_FRAME / AX_RATE; }

// How many AX frames are due by the game clock, plus the lead the ring is kept at.
static int FramesDue(void) {
    u64 want = (u64)((PortTicks() - sClockStart) / FrameTicks()) + RING_TARGET / AX_FRAME;
    if (want <= sFramesDone) return 0;
    u64 due = want - sFramesDone;
    if (due > 60) { sFramesDone = want - 60; due = 60; }             // far behind (180 ms): skip, do not catch up
    sFramesDone += due;
    return (int)due;
}

s64 PortAudioNextTicks(void) {
    if (!sStarted) return 0;
    s64 lead = RING_TARGET / AX_FRAME;
    return sClockStart + ((s64)sFramesDone + 1 - lead) * FrameTicks();
}

static void Output(const s16* frame) {
    if (sWav) {
        fwrite(frame, 4, AX_FRAME, sWav);
        sWavSamples += AX_FRAME;
        if (sWavSamples % (AX_RATE / 2 / AX_FRAME * AX_FRAME) < AX_FRAME) WavHeader();
    }
    if (!sWave || PortFastActive()) return;       // fast-forward: the device is not fed
    LONG wr = sRingWrite;
    if (wr - sRingRead > RING_SAMPLES / 2) { sStats.dropped++; return; }   // the device is behind: drop
    s32 gain = gPortSettings.mute ? 0 : gPortSettings.volume;             // the settings screen's volume: the device only, not --wav
    for (int n = 0; n < AX_FRAME; n++) {
        s16* p = &sRing[((u32)(wr + n) % RING_SAMPLES) * 2];
        p[0] = (s16)(frame[n * 2] * gain / 100); p[1] = (s16)(frame[n * 2 + 1] * gain / 100);
    }
    sRingWrite = wr + AX_FRAME;
}

void PortAudioTick(void) {
    if (!sAxInit || gPortOptions.selfTest) return;
    if (!sStarted) Start();
    int due = FramesDue();
    for (int i = 0; i < due; i++) {
        if (sAxCallback) sAxCallback();
        s16 frame[AX_FRAME * 2];
        MixFrame(frame);
        sStats.frames++;
        sStats.samplesOut += AX_FRAME;
        Output(frame);
    }
}

u32 PortAudioVoices(u32* outOf) {
    u32 n = 0;
    for (int i = 0; i < AX_VOICES; i++) if (sRun[i].running) n++;
    if (outOf) *outOf = AX_VOICES;
    return n;
}

const char* PortAudioStats(char* buf, u32 n) {
    if (sWav) WavHeader();
    snprintf(buf, n, "%u AX frame(s) (%.1f s), %u voice start(s), peak level %u of 32767%s%s",
             sStats.frames, sStats.frames * 0.003, sStats.started, sStats.peak,
             sWave ? ", played on the sound device" : ", no sound device", sWav ? ", written to the --wav file" : "");
    if (sStats.dropped) snprintf(buf + strlen(buf), n - strlen(buf), ", %u frame(s) dropped", sStats.dropped);
    return buf;
}

// ------------------------------------------------------------------ self-test (no game data, no device)
int PortSelfTestAudio(void) {
    int fails = 0;
#define CHECK(cond) do { if (!(cond)) { fails++; PortLog("self-test FAILED: %s (line %d)", #cond, __LINE__); } } while (0)
    u8* mem = (u8*)(uintptr_t)(PORT_MEM2_BASE + 0x100000);
    u32 phys = (u32)(uintptr_t)mem & 0x7FFFFFFFu;
    s16 out[AX_FRAME * 2];
    AXInit();
    MIXInit();
    MIXSetSoundMode(1);

    // PCM16, big-endian, 8 samples, not looping, full level to the left.
    for (int i = 0; i < 8; i++) { mem[i * 2] = (u8)((i * 1000) >> 8); mem[i * 2 + 1] = (u8)(i * 1000); }
    u8* v = (u8*)(uintptr_t)AXAcquireVoice(16, NULL, NULL);
    CHECK(v != NULL);
    u16 addr[8] = {0, 0x0A, 0, 0, 0, 0, 0, 0};
    u32 first = phys / 2, last = first + 7;
    addr[2] = (u16)(first >> 16); addr[3] = (u16)first; addr[4] = (u16)(last >> 16); addr[5] = (u16)last;
    addr[6] = (u16)(first >> 16); addr[7] = (u16)first;
    u16 src[7] = {1, 0, 0, 0, 0, 0, 0};
    AXSetVoiceAddr(v, addr);
    AXSetVoiceSrc(v, src);
    MIXInitChannel(v, 0, 0, -904, -904, -904, 0, 127, 0);
    AXSetVoiceState(v, 1);
    MixFrame(out);
    CHECK(out[2] == 0 && out[4] == 1000 && out[16] == 7000 && out[18] == 0 && out[5] == 0);
    CHECK(H(v, V_STATE) == 0);                                       // it ran out and stopped
    CHECK(MIXIsMute(v) == 0);
    AXFreeVoice(v);

    // DSP-ADPCM: one block, predictor 0 with scale 2 and coefficient pair 0 = (2048, 0), so each
    // sample is the one before plus four times its nibble; looping.
    memset(mem, 0, 16);
    mem[0] = 0x02; mem[1] = 0x12; mem[2] = 0xF0;                     // nibbles 1, 2, -1, 0...
    v = (u8*)(uintptr_t)AXAcquireVoice(16, NULL, NULL);
    u32 base = phys * 2;
    u16 a2[8] = {1, 0, (u16)((base + 2) >> 16), (u16)(base + 2), (u16)((base + 15) >> 16), (u16)(base + 15),
                 (u16)((base + 2) >> 16), (u16)(base + 2)};
    u16 coef[20] = {2048, 0};
    coef[17] = 0x02;
    AXSetVoiceAddr(v, a2);
    AXSetVoiceAdpcm(v, coef);
    AXSetVoiceSrc(v, src);
    MIXInitChannel(v, 0, 0, -904, -904, -904, 127, 127, 0);
    AXSetVoiceState(v, 1);
    MixFrame(out);
    CHECK(out[1] == 0 && out[3] == 4 && out[5] == 12 && out[7] == 8 && out[2] == 0);
    CHECK(H(v, V_STATE) == 1);                                       // still running: it loops
    AXFreeVoice(v);

    // AXART: a pitch articulator of +1200 cents doubles the ratio; a volume of -60 (6 dB) halves the level.
    u8 sound[0x14] = {}, pitch[0x0C] = {}, vol[0x0C] = {};
    v = (u8*)(uintptr_t)AXAcquireVoice(16, NULL, NULL);
    AXARTInit();
    AXARTInitSound(sound, (s32)(uintptr_t)v, 16000);
    AXARTInitArtPitch(pitch); *(s32*)(pitch + 8) = 1200 << 16;
    AXARTInitArtVolume(vol);  *(s32*)(vol + 8) = -60 * 65536;
    AXARTAddArticulator(sound, pitch);
    AXARTAddArticulator(sound, vol);
    AXARTAddSound(sound);
    CHECK(Get32(v, V_RATIO) == 0x10000);
    AXARTServiceSounds();
    MIXUpdateSettings();
    CHECK(H(v, V_MIX) > 11500 && H(v, V_MIX) < 11700 && H(v, V_MIX) == H(v, V_MIX + 4));   // -6 dB, -3 dB centre
    CHECK(AXARTRemoveArticulator(sound, vol) == 1 && AXARTRemoveArticulator(sound, vol) == 0);
    AXARTRemoveSound(sound);
    AXFreeVoice(v);
    AXQuit();
    memset(&sStats, 0, sizeof(sStats));
    return fails;
}

#endif
