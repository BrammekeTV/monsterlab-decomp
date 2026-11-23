// Audio / movie functions in 0x8020: audio frame update (stream + voice state machines), DSP voice
// setup from a .dsp header, music (.dsp) loading, and the Bink movie player.
#include <stdint.h>
#include "types.h"
#include "revolution.h"
#include "engine/rend.h"
#include "system/sys.h"

extern "C++" s32 gTextLanguage;   // 0x805F81B4 (defined in game_flow.cpp)
extern "C" void fn_80142970(void);   // 0x80142970
extern "C++" void fn_8015D530(void);   // 0x8015D530 (C++ linkage, small_8015.cpp)
extern "C++" void fn_8015DDEC(void);   // 0x8015DDEC (C++ linkage, small_8015.cpp)

extern "C" {
void   DCFlushRange(void* addr, u32 size);
void   GameLog(int level, int category, const char* fmt, ...);
void*  MEMAllocFromAllocator(MEMAllocator* allocator, u32 size);
s32    OSDisableInterrupts(void);                              // 0x802393C4
s32    OSRestoreInterrupts(s32 level);                         // 0x802393EC
s32    DVDGetDriveStatus(void);                                // 0x8025394C
void   RendBeginFrame(void);
void   RendClear(int flags, int a, int b, f32 depth);
void   RendEndFrame(void);
void   SetRenderEnabled(int on);
s32    VideoGetHeight(RendVideo* video);
s32    VideoGetWidth(RendVideo* video);
u8*    fn_80142500(void* p);                                   // 0x80142500
f32    TimerReset(u8* timer);                                  // 0x80142744
                                      // 0x80142970
void   fn_80142ED0(u8* base, s32 size);                        // 0x80142ED0
s32    fn_80146808(u8* file, const char* name, const char* mode);
s32    fn_80146930(u8* file, void* buf, s32 size);
void   fn_801469FC(u8* file);
s32    fn_80146A58(u8* file);
s32    fn_8014698C(u8* p, s32 a, s32 b);                       // 0x8014698C
BOOL   fn_80147554(s32 button);
void   fn_80150148(void);
void   fn_80150DEC(void);
void   fn_80151014(void);
void   fn_80151244(void);

void   fn_8015D578(void);
void   fn_8015D5A4(void);

void   fn_80188E5C(u8* p);
void   fn_80188ED4(s32 a);
void*  fn_801B7198(void* anim, s32 a);                         // 0x801B7198
s32    fn_801C63E4(const char* name);                          // 0x801C63E4
void   fn_801C64E8(void);                                      // 0x801C64E8
BOOL   fn_801E6BD0(u8* obj, u32* handle);
void   fn_801FC0A0(void);
void   fn_801FC1EC(void);
void   fn_801FC70C(void);                                      // 0x801FC70C
s32    fn_801FCA38(f32 x, f32 y);
s32    fn_801FD420(u8* audio, s32 idx, s32 pan);
s32    fn_801FE52C(u8* audio, void* desc, s32 c);              // 0x801FE52C
void   fn_801FF2A8(u8* audio, s32 handle);                     // 0x801FF2A8
void*  IdPoolNext(void* pool);                                 // 0x801E6EB0
void   fn_80201D9C(u8* audio, u8* stream);                     // 0x80201D9C
s32    fn_8020151C(u8* slot);                                  // 0x8020151C
void   fn_802019C4(u8* voice);                                 // 0x802019C4
void   fn_80203220(void);                                      // 0x80203220
void   fn_80203DE8(u8* audio, u8* state);
void   fn_80204838(u8* bink);                                  // 0x80204838
s32    fn_80205454(u8* state);                                 // 0x80205454
void   fn_80205894(u8* state, f32 x, f32 y, f32 a, f32 b, f32 c, s32 w, s32 h, s32 flag);   // 0x80205894
void   fn_802062F0(u8* state);                                 // 0x802062F0
void   fn_8020506C(u8* p);
void   fn_80206308(u8* p);
void   GXSetCopyClear(u8* color, u32 z);
s32    DVDOpen(char* path, u8* p);
void   DVDClose(u8* p);
void   fn_80253A48(u8* p, s32 a);                              // 0x80253A48
void   AXFreeVoice(u8* voice);                                 // 0x80257FA8
void   AXSetVoicePriority(u8* voice, u32 a);                          // 0x802581A4
s32    AXSetVoiceSrcType(u8* voice, s32 a);                          // 0x8025AC30
void   AXSetVoiceType(u8* voice, s32 a);                   // 0x8025AD60
s32    AXSetVoiceMix(u8* voice, u16* p);                         // 0x8025ADAC
void   AXSetVoiceState(u8* voice, s32 a);                          // 0x8025ACF0
s32    AXSetVoiceAddr(u8* voice, u16* p);                         // 0x8025B0A4
s32    AXSetVoiceAdpcm(u8* voice, u16* p);                         // 0x8025B19C
s32    AXSetVoiceSrc(u8* voice, u16* p);                         // 0x8025B234
s32    AXSetVoiceAdpcmLoop(u8* voice, u16* p);                         // 0x8025B330
s32    AXSetVoiceRmtOn(u8* voice, s32 a);                          // 0x8025B3A0
void   fn_8025B390(void);                                      // 0x8025B390
void   fn_8025B398(void);                                      // 0x8025B398
void   MIXReleaseChannel(u8* voice);                                 // 0x80262194
void   MIXMute(u8* voice);                                 // 0x80262320
void   MIXUnMute(u8* voice);                                 // 0x80262344
s32    MIXIsMute(u8* voice);                                 // 0x80262368
void   AXARTServiceSounds(void);                                      // 0x80263FAC
void   AXARTRemoveSound(u8* p);
void   MIXUpdateSettings(void);                                      // 0x802623A8
s32    WPADProbe(s32 chan, u32* out);
s32    WPADControlSpeaker(s32 chan, s32 a, void* cb);
s32    fn_802B47A0(s32 a, s32* b);                             // 0x802B47A0
s32    fn_802B4A1C(u8* bink, u8* p);                           // 0x802B4A1C
s32    fn_802B4B14(u8* bink, u8* p);                           // 0x802B4B14
u8*    fn_802B4B30(const char* path, u32 flags);               // 0x802B4B30
s32    fn_802B5C6C(u8* bink);                                  // 0x802B5C6C
s32    fn_802B61D0(u8* bink);                                  // 0x802B61D0
s32    fn_802B625C(u8* bink);                                  // 0x802B625C
s32    fn_802B6574(u8* bink, s32 a, s32 b);                    // 0x802B6574
s32    fn_802B6798(u8* bink);                                  // 0x802B6798
s32    fn_802B6950(u8* bink);                                  // 0x802B6950
s32    fn_802B6C60(u8* bink, s32 a, s32 b);                    // 0x802B6C60
f64    fn_8033F20C(f64 x, f64 y);                              // 0x8033F20C (pow)
void*  memset(void* p, int c, unsigned long n);
char*  strcat(char* dst, const char* src);
char*  strcpy(char* d, const char* s);
u32    strlen(const char* s);
char*  strncpy(char* dst, const char* src, size_t n);

extern MEMAllocator lbl_803F2870;   // 0x803F2870
extern u8   gAudio[];               // 0x8047C7F0
extern u8   gIdPool[];              // 0x8048A8F0
extern u8   lbl_804FDAA0[];         // 0x804FDAA0
extern u8   lbl_8051DAD0[];         // 0x8051DAD0 (two music stream blocks, 0xC0 each)
extern u8   lbl_8051E6A8[];         // 0x8051E6A8 (sound voices, 0x114 each)
extern u8   lbl_80524E28[];         // 0x80524E28 (sound table entries, 0x58 each, 0x60 of them)
extern u8   lbl_80526F28[];         // 0x80526F28 (movie player state)
extern s32  gScreenHeight;          // 0x805F938C
extern s32  gScreenWidth;           // 0x805F9388
extern u32  lbl_805F98E0;           // 0x805F98E0
extern u8   lbl_805F98C1;           // 0x805F98C1
extern u8   lbl_805FB470[];         // 0x805FB470 (function-pointer slot, called with 0)
extern s32  lbl_805FB51C;           // 0x805FB51C
extern s32  lbl_805FB630;           // 0x805FB630
extern s32  lbl_805FB6B8;           // 0x805FB6B8
extern s32  lbl_805FBA88;           // 0x805FBA88
extern s32  lbl_805FBA8C;           // 0x805FBA8C
extern void* lbl_805FBA90;          // 0x805FBA90
extern void* lbl_805FBA94;          // 0x805FBA94 (id from gIdPool)
extern s32  lbl_805FBA98;           // 0x805FBA98
extern s32  lbl_805FBAA0;           // 0x805FBAA0 (last DVD drive status)
extern s32  lbl_805FBAB0;           // 0x805FBAB0
extern s32  lbl_805FBAB4;           // 0x805FBAB4
extern s32  lbl_805FBABC;           // 0x805FBABC (count of unpaused looping voices)
extern s8   lbl_805FBAC4;           // 0x805FBAC4
extern s32  lbl_805FBAC8;           // 0x805FBAC8
extern void* lbl_805FBACC;          // 0x805FBACC
extern s32  lbl_805FBAD0;           // 0x805FBAD0
extern void* lbl_805FBADC;          // 0x805FBADC
extern s32  lbl_805FBAE0;           // 0x805FBAE0 (audio busy flag)
extern s8   lbl_805FBB00;           // 0x805FBB00
extern u8   lbl_805FBB08[];         // 0x805FBB08 (timer)
extern u8*  lbl_805FBB18;           // 0x805FBB18 (current Bink handle)
extern u8*  lbl_805FBB80[2];        // 0x805FBB80 (aligned DSP data per slot)
extern u8*  lbl_805FBB88[2];        // 0x805FBB88 (raw DSP allocation per slot)
extern s32  lbl_805FBB90;           // 0x805FBB90
}

extern "C" {

// Releases a sound-table slot (0x58 bytes) and the voice it points at: clears the voice's back
// reference, drops the animation link at +0x44 and resets the slot to its idle values.
// Inlined at two places in the original audio update.
static void SoundSlotFree(u8* p, u8* voices) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    if (p == NULL || *(s32*)(p + 0x0) == -1) {
        lbl_805FBAE0 = saved;
        return;
    }
    s32 voiceIdx = *(s32*)(p + 0x8);
    if (voiceIdx != -1) {
        *(s32*)(voices + voiceIdx * 0x114 + 0xC) = 0;
    }
    u32 anim = *(u32*)(p + 0x44);
    u8* a = (u8*)fn_801B7198(&anim, 0);
    if (a != NULL && *(s32*)(a + 0xAC) == *(s32*)(p + 0x0)) {
        *(s32*)(a + 0xAC) = -1;
    }
    *(s32*)(p + 0x0) = -1;
    *(s32*)(p + 0x4) = -1;
    *(s32*)(p + 0x8) = -1;
    *(s32*)(p + 0xC) = 0;
    *(s32*)(p + 0x10) = 0;
    *(s32*)(p + 0x14) = -1;
    *(s32*)(p + 0x44) = lbl_805FB6B8;
    *(s32*)(p + 0x4C) = 0;
    *(s32*)(p + 0x50) = 0;
    *(s32*)(p + 0x30) = 0;
    *(f32*)(p + 0x34) = 0.0f;
    *(s32*)(p + 0x54) = 0;
    lbl_805FBAE0 = saved;
}

// 0x80201F38
// Per-frame audio update (runs as a callback with interrupts off): tracks the DVD drive status,
// steps the two music-stream state machines (buffer refill, loop wrap, stop), retires finished
// sound-table slots, steps the 0x60 voices (start, stop, free) and refreshes per-voice volume.
void fn_80201F38(void) {
    u8* base = lbl_804FDAA0 + 0x20000;                 // 0x8051DAA0
    u8* voices = lbl_8051E6A8;
    s32 irq = OSDisableInterrupts();
    if (lbl_805FBAE0 != 0) {
        OSRestoreInterrupts(irq);
        return;
    }
    if (lbl_805FBB00 == 0) {
        fn_80142500(lbl_805FBB08);
        lbl_805FBB00 = 1;
    }
    fn_8025B398();
    fn_8025B390();
    TimerReset(lbl_805FBB08);
    if (lbl_805FBAC4 != 0) {
        if (WPADProbe(0, NULL) == 0) {
            lbl_805FBAC4 = 0;
            fn_80203220();
        }
    }
    if (lbl_805FBAC8 > 0) {
        lbl_805FBAC8--;
    } else if (lbl_805FBACC != NULL) {
        WPADControlSpeaker(0, lbl_805FBAD0, lbl_805FBACC);
        lbl_805FBACC = NULL;
    }

    // Drive status: 0..2 are the "ready" states (jump table), everything else counts as paused.
    lbl_805FBAA0 = DVDGetDriveStatus();
    s32 driveBusy = (lbl_805FBAA0 >= 0 && lbl_805FBAA0 <= 2) ? 0 : 1;
    lbl_805FBAB4 = driveBusy;
    lbl_805FBAB0 = driveBusy;

    // The two music streams.
    for (s32 i = 0; i < 2; i++) {
        u8* s = lbl_8051DAD0 + i * 0xC0;
        u32 state = *(u32*)(s + 0xB4);
        if (state > 8) continue;
        switch (state) {
        case 0:
        case 1:
            break;
        case 7:
            if (*(s32*)(s + 0xB8) != 0) break;
            if (lbl_805FBAB4 != 0) break;
            *(s32*)(s + 0x80) = *(s32*)(s + 0x34);
            *(s32*)(s + 0x84) = 0;
            *(s32*)(s + 0xA4) = 0;
            *(s32*)(s + 0xBC) = 0;
            fn_80201D9C(gAudio, s);
            *(s32*)(s + 0xB4) = 8;
            break;
        case 8:
            if (lbl_805FBAB4 != 0) {
                *(s32*)(s + 0xB4) = 7;
                break;
            }
            if (*(s32*)(s + 0xB8) != 0) break;
            if (*(s32*)(s + 0xBC) == 0) break;
            *(s32*)(s + 0xB4) = 0;
            if (*(s32*)(s + 0x9C) != 0) break;
            fn_801FD420(gAudio, 0, 0);
            fn_801FD420(gAudio, 1, 0x7F);
            break;
        case 2: {
            // Playing: work out the DSP read position and refill / wrap the ring buffer.
            u8* voice = *(u8**)(s + 0x8C);
            u32 pos = ((u32)*(u16*)(voice + 0xA2) << 16) | *(u16*)(voice + 0xA4);
            *(u32*)(s + 0xB0) = pos;
            u32 fmt = *(u16*)(s + 0x78);
            if (fmt == 0xA) {
                *(u32*)(s + 0xB0) = *(u32*)(s + 0xB0) << 1;
            } else if (fmt == 0) {
                *(u32*)(s + 0xB0) = *(u32*)(s + 0xB0) >> 1;
            }
            u32 cur = *(u32*)(s + 0xB0);
            bool hitEnd = false;
            if (*(s32*)(s + 0x7C) != 0) {
                u32 prev = *(u32*)(s + 0xAC);
                u32 mark = *(u32*)(s + 0x88);
                if (cur < prev) {
                    hitEnd = (prev < mark) || (mark <= cur);
                } else {
                    hitEnd = (prev < cur) && (prev < mark) && (mark <= cur);
                }
            }
            if (hitEnd) {
                // The stream reached its end mark: stop it.
                if (lbl_805F98E0 != 0 && *(s32*)(s + 0x9C) == 0) {
                    if (*(s32*)(s + 0xB8) != 0) {
                        fn_80253A48(s, 0);
                        fn_80253A48(s + 0x3C, 0);
                    }
                    *(s32*)(s + 0xB4) = 5;
                    *(u8*)(gAudio + 0x29) = 0;
                    if (lbl_805F98C1 != 0) {
                        *(s32*)(s + 0x80) = *(s32*)(s + 0x34);
                    } else {
                        *(s32*)(s + 0x80) = *(s32*)(gAudio + 0x9C);
                        fn_8014698C(gAudio + 0x3C, 0, 0);
                        fn_8014698C(gAudio + 0x1E4, 0, 0);
                    }
                    *(s32*)(s + 0x7C) = 0;
                    *(s32*)(s + 0x84) = 0;
                } else {
                    fn_80203DE8(gAudio, s);
                }
                u8* v0 = *(u8**)(s + 0x8C);
                if (v0 != NULL) AXSetVoiceState(v0, 0);
                u8* v1 = *(u8**)(s + 0x90);
                if (v1 != NULL) AXSetVoiceState(v1, 0);
                break;
            }
            u32 prev = *(u32*)(s + 0xAC);
            if (cur < prev) {
                // Play position wrapped around the ring buffer: refill the half just played.
                if (*(s32*)(s + 0xB8) != 0) break;
                if (lbl_805FBAB4 != 0) break;
                *(s32*)(s + 0xA4) = 0x8000;
                fn_80201D9C(gAudio, s);
                *(u32*)(s + 0xAC) = *(u32*)(s + 0xB0);
            } else {
                u32 half = *(u32*)(s + 0xA8);
                if (prev < half && cur >= half) {
                    // Crossed the half-way mark: refill the first half.
                    if (*(s32*)(s + 0xB8) != 0) break;
                    if (lbl_805FBAB4 != 0) break;
                    *(s32*)(s + 0xA4) = 0;
                    fn_80201D9C(gAudio, s);
                    *(u32*)(s + 0xAC) = *(u32*)(s + 0xB0);
                } else {
                    *(u32*)(s + 0xAC) = cur;
                }
            }
            break;
        }
        case 5:
            if (*(s32*)(s + 0xB8) != 0) break;
            if (lbl_805FBAB4 != 0) break;
            if (DVDGetDriveStatus() != 0) break;
            memset(*(u8**)(s + 0x94), 0, 0x10000);
            memset(*(u8**)(s + 0x98), 0, 0x10000);
            DCFlushRange(*(u8**)(s + 0x94), 0x10000);
            DCFlushRange(*(u8**)(s + 0x98), 0x10000);
            *(s32*)(s + 0x84) = 0;
            *(s32*)(s + 0xA4) = 0;
            *(s32*)(s + 0xBC) = 0;
            fn_80201D9C(gAudio, s);
            *(s32*)(s + 0xB4) = 6;
            break;
        case 6: {
            if (lbl_805FBAB4 != 0) {
                *(s32*)(s + 0xB4) = 5;
                break;
            }
            if (*(s32*)(s + 0xB8) != 0) break;
            if (*(s32*)(s + 0xBC) == 0) break;
            *(u32*)(s + 0xAC) = *(u32*)(s + 0xA8);
            // Point both DSP voices at the two stream buffers and restart them.
            for (s32 ch = 0; ch < 2; ch++) {
                u8* buf = *(u8**)(s + 0x94 + ch * 4);
                u8* voice = *(u8**)(s + 0x8C + ch * 4);
                uintptr_t bufPtr = (uintptr_t)buf;
                u32 addr = *(u32*)(buf + 0x18) + (((u32)bufPtr + 0x80000000u) << 1) + 0xC0;
                *(u16*)(voice + 0xA2) = (u16)(addr >> 16);
                *(u16*)(voice + 0xA4) = (u16)addr;
                *(u32*)(voice + 0x1C) |= 0x400;
            }
            AXSetVoiceState(*(u8**)(s + 0x8C), 1);
            AXSetVoiceState(*(u8**)(s + 0x90), 1);
            *(u8*)(gAudio + 0x29) = 1;
            *(s32*)(s + 0xB4) = 2;
            break;
        }
        case 3: {
            u8* v0 = *(u8**)(s + 0x8C);
            if (v0 != NULL) MIXReleaseChannel(v0);
            u8* v1 = *(u8**)(s + 0x90);
            if (v1 != NULL) MIXReleaseChannel(v1);
            *(s32*)(s + 0xB4) = 4;
            break;
        }
        case 4:
            if (*(s32*)(s + 0xB8) != 0 && lbl_805FBAB4 == 0) break;
            if (lbl_805F98C1 != 0) {
                DVDClose(s);
                DVDClose(s + 0x3C);
            } else {
                fn_801469FC(gAudio + 0x3C);
                fn_801469FC(gAudio + 0x1E4);
            }
            *(s32*)(s + 0xB4) = 0;
            break;
        }
    }

    // Pending sound request: when the queued id changes, hand the new request to the mixer.
    uintptr_t nextId = (uintptr_t)IdPoolNext(gIdPool);
    uintptr_t curId = (uintptr_t)lbl_805FBA94;
    if (curId != nextId) {
        u32 handle = (u32)curId;
        if (fn_801E6BD0(gIdPool, &handle) != 0) {
            *(void**)(*(u8**)(base + 0x0) + 0x4) = lbl_805FBA90;
            s32 want = lbl_805FBA8C;
            if (want != 0) {
                s32 saved = lbl_805FBAE0;
                lbl_805FBAE0 = 1;
                for (s32 i = 0; i < 0x60; i++) {
                    u8* slot = lbl_80524E28 + i * 0x58;
                    if (*(s32*)(slot + 0xC) == want) {
                        fn_801FF2A8(gAudio, *(s32*)slot);
                        lbl_805FBAE0 = saved;
                        goto requestDone;
                    }
                }
                lbl_805FBAE0 = saved;
            }
        requestDone:
            lbl_805FBA8C = *(s32*)(base + 0x0);
            fn_801FE52C(gAudio, base, lbl_805FBA98);
            lbl_805FBA94 = IdPoolNext(gIdPool);
        }
    }

    // Sound-table slots: free the ones whose voice is gone, step the ones with a voice.
    for (s32 i = 0; i < 0x60; i++) {
        u8* slot = lbl_80524E28 + i * 0x58;
        if (*(s32*)(slot + 0x10) == 0) continue;
        s32 voiceIdx = *(s32*)(slot + 0x8);
        if (voiceIdx == -1) {
            SoundSlotFree(slot, voices);
        } else {
            u8* v = voices + voiceIdx * 0x114;
            fn_802019C4(v);
            if (*(s32*)(v + 0x8) != 0) continue;
            if (*(s32*)(slot + 0x10) == 8) continue;
            if (fn_8020151C(slot) != 0) {
                *(s32*)(voices + *(s32*)(slot + 0x8) * 0x114 + 0x4) = 4;
            }
        }
    }

    // The first two voices carry the music streams: start or stop their DSP voices.
    if (*(s8*)(gAudio + 0x28) != 0) {
        if (*(s8*)(gAudio + 0x29) != 0 && lbl_805FBAB4 == 0) {
            for (s32 i = 0; i < 2; i++) {
                u8* v = voices + i * 0x114;
                u8* h = *(u8**)v;
                if (h == NULL) continue;
                if (*(u16*)(h + 0x38) == 0) {
                    MIXUnMute(h);
                    AXSetVoiceState(*(u8**)v, 1);
                }
                if (lbl_805F98E0 != 0) continue;
                u8* hh = *(u8**)v;
                u32 cur = ((u32)*(u16*)(hh + 0xA2) << 16) | *(u16*)(hh + 0xA4);
                u32 end = ((u32)*(u16*)(hh + 0x9E) << 16) | *(u16*)(hh + 0xA0);
                if (end - cur <= 0x7C) {
                    *(u8*)(gAudio + 0x29) = 0;
                }
            }
        } else {
            for (s32 i = 0; i < 2; i++) {
                u8* v = voices + i * 0x114;
                u8* h = *(u8**)v;
                if (h != NULL && *(u16*)(h + 0x38) == 1) {
                    MIXMute(h);
                    AXSetVoiceState(*(u8**)v, 0);
                }
            }
        }
    }

    // Voice state machine (0x60 voices): 1 = start, 2 -> 3, 3/4 = wait for the DSP to finish, then free.
    s32 finished = 0;
    for (u32 i = 0; i < 0x60; i++) {
        u8* v = voices + i * 0x114;
        u8* h = *(u8**)v;
        if (h == NULL) continue;
        if (*(s32*)(v + 0xC) == 0) continue;
        s32 vs = *(s32*)(v + 0x4);
        if (vs == 2) {
            *(s32*)(v + 0x4) = 3;
        } else if (vs == 1) {
            AXSetVoiceState(h, 1);
            *(s32*)(v + 0x4) = 2;
        } else if (vs == 4) {
            finished = 1;
        } else if (vs == 3) {
            if (*(u16*)(h + 0x38) == 0) finished = 1;
        }
        if (finished != 0) {
            if (MIXIsMute(*(u8**)v) == 0) {
                MIXMute(*(u8**)v);
            }
        }
        if (finished != 0) {
            if (MIXIsMute(*(u8**)v) != 0) {
                if (i > 1) {
                    AXSetVoiceState(*(u8**)v, 0);
                    if (*(u8*)(v + 0xFE) != 0 || *(u8*)(v + 0xFC) != 0) {
                        AXARTRemoveSound(v + 0x10);
                    }
                    if (*(u8*)(v + 0xFC) != 0) {
                        if (*(u8*)(v + 0xFD) != 0) {
                            lbl_805FBABC--;
                            if (lbl_805FBABC < 0) lbl_805FBABC = 0;
                            *(u8*)(v + 0xFD) = 0;
                        }
                        *(u8*)(v + 0xFC) = 0;
                    }
                    AXFreeVoice(*(u8**)v);
                    *(u8**)v = NULL;
                    *(s32*)(v + 0x4) = 0;
                    SoundSlotFree(*(u8**)(v + 0xC), voices);
                    *(s32*)(v + 0xC) = 0;
                } else {
                    AXSetVoiceState(*(u8**)v, 0);
                    AXFreeVoice(*(u8**)v);
                    *(u8**)v = NULL;
                    *(s32*)(v + 0x4) = 0;
                }
                finished = 0;
            }
        }
    }

    // Volume refresh for the sound voices (index 2 and up): level in 1/10 dB from +0x4C to a 1..31 gain.
    for (u32 i = 2; i < 0x60; i++) {
        u8* v = voices + i * 0x114;
        if (*(u8**)v == NULL) continue;
        s32 level = *(s32*)(v + 0x4C) >> 16;
        f32 x = (f32)level;
        f32 exponent = (x / 10.0f) / 30.0f;
        f32 amp = (f32)fn_8033F20C(10.0, (f64)exponent);
        if (amp > -0.0001f && amp < 0.0001f) amp = 0.0f;
        u32 gain = *(u32*)(v + 0x100) * (u32)amp;
        if (gain < 1) gain = 1;
        else if (gain > 0x1F) gain = 0x1F;
        AXSetVoicePriority(*(u8**)v, gain);
    }
    AXARTServiceSounds();
    MIXUpdateSettings();
    OSRestoreInterrupts(irq);
    lbl_805FBA88 = 1;
    fn_80142ED0((u8*)&lbl_805FBA88, 4);
}

// 0x80203340
// Fills the DSP voice's parameter blocks from a Wii .dsp header (ADPCM/PCM8/PCM16 format, start/loop/
// end addresses in DSP nibble units, ADPCM coefficients, pitch ratio from the sample rate / 32000).
// 'streamed' (or a header loop flag) selects the header's own loop address instead of the shared
// buffer at lbl_805FBADC.
void fn_80203340(u8* hdr, u8* voice, s32 streamed) {
    u16 addrBlk[8];      // 0x20: loop flag, format, loop/end/current addresses (hi, lo)
    u16 loopBlk[3];      // 0x08: ADPCM loop state (predictor scale, history 1, history 2)
    u16 srcBlk[7];       // 0x10: pitch ratio (hi, lo), then zeroed counters
    u16 coefBlk[20];     // 0x60: ADPCM coefficients
    u16 zeroBlk[24];     // 0x30

    addrBlk[0] = *(u16*)(hdr + 0xC);
    u16 fmt = *(u16*)(hdr + 0xE);
    addrBlk[1] = fmt;
    if (fmt != 0xA && fmt != 0x19 && fmt != 0) {
        GameLog(4, 2, "Invalid WiiDSP data - not PCM16, PCM8, or ADPCM. Did you perhaps just add a 32-bit PCM sound?");
        return;
    }
    // Physical address of the sample data, in DSP units (2 per byte), 0x60 bytes past the header.
    uintptr_t hdrPtr = (uintptr_t)hdr;
    u32 dataBase = (u32)hdrPtr + 0x80000060u;
    u32 loopAddr;
    loopBlk[0] = 0;
    loopBlk[1] = 0;
    loopBlk[2] = 0;
    if (addrBlk[0] != 0 || streamed != 0) {
        addrBlk[0] = 1;
        loopBlk[0] = *(u16*)(hdr + 0x44);
        loopBlk[1] = *(u16*)(hdr + 0x46);
        loopBlk[2] = *(u16*)(hdr + 0x48);
        loopAddr = *(u32*)(hdr + 0x10) + dataBase * 2;
    } else {
        uintptr_t sharedPtr = (uintptr_t)lbl_805FBADC;
        loopAddr = (((u32)sharedPtr + 0x80000000u) << 1) + 2;
    }
    addrBlk[2] = (u16)(loopAddr >> 16);
    addrBlk[3] = (u16)loopAddr;
    u32 endAddr = *(u32*)(hdr + 0x14) + dataBase * 2;
    addrBlk[5] = (u16)endAddr;
    addrBlk[4] = (u16)(endAddr >> 16);
    u32 curAddr = *(u32*)(hdr + 0x18) + dataBase * 2;
    addrBlk[7] = (u16)curAddr;
    addrBlk[6] = (u16)(curAddr >> 16);
    for (s32 i = 0; i < 20; i++) {
        coefBlk[i] = *(u16*)(hdr + 0x1C + i * 2);
    }
    u32 ratio = (u32)(65536.0f * ((f32)*(u32*)(hdr + 0x8) / 32000.0f));
    srcBlk[1] = (u16)ratio;
    srcBlk[0] = (u16)(ratio >> 16);
    srcBlk[2] = 0;
    srcBlk[3] = 0;
    srcBlk[4] = 0;
    srcBlk[5] = 0;
    srcBlk[6] = 0;
    AXSetVoiceType(voice, 0);  // the asm passes only r3/r4
    AXSetVoiceAddr(voice, addrBlk);
    AXSetVoiceAdpcm(voice, coefBlk);
    AXSetVoiceAdpcmLoop(voice, loopBlk);
    AXSetVoiceRmtOn(voice, 0);
    AXSetVoiceSrcType(voice, 1);
    AXSetVoiceSrc(voice, srcBlk);
    // The original copies 24 halfwords from the voice (+0x3C..) and then clears them before the call.
    memset(zeroBlk, 0, 0x30);
    AXSetVoiceMix(voice, zeroBlk);
}

// 0x80203664
// Loads "<name>.dsp" for music stream 'slot' (0/1) into an aligned buffer. With lbl_805F98C1 set the file
// is opened through DVDOpen instead. Returns 0 on success, -1 on failure.
s32 fn_80203664(u8* audio, const char* name, s32 slot) {
    char buf[0x80];
    s32 room = 0x7F - (s32)strlen(".dsp");
    strncpy(buf, name, room - 1);
    buf[room - 1] = 0;
    strcat(buf, ".dsp");
    if (*(s32*)(lbl_8051DAD0 + 0xB4) != 0) {
        fn_80203DE8(audio, lbl_8051DAD0);
    }
    GameLog(1, 2, "SOUNDDEBUG: LoadMusicAsync %s\n", buf);
    GameLog(1, 2, "SOUNDDEBUG: Trying to stream %s\n", buf);
    u8* file = NULL;
    if (lbl_805F98C1 != 0) {
        if (slot == 0) {
            if (DVDOpen(buf, lbl_8051DAD0) == 0) return -1;
        } else if (slot == 1) {
            if (DVDOpen(buf, lbl_8051DAD0 + 0x3C) == 0) return -1;
        } else {
            return -1;
        }
    } else {
        file = (slot != 0) ? audio + 0x20 : audio + 0x1C8;
        fn_80146808(file + 0x1C, buf, "rb");
        s32 size = fn_80146A58(file + 0x1C);
        *(s32*)(file + 0x7C) = size;
        *(s32*)(file + 0x14) = 0;
        *(s32*)(file + 0x10) = 0;
        *(s32*)(file + 0x18) = 0;
        if (size == 0) return -1;
        *(u8*)(file + 0x8) = 1;
    }
    lbl_805FBB90 = 0x10020;
    u8* mem = (u8*)MEMAllocFromAllocator(&lbl_803F2870, 0x10020);
    lbl_805FBB88[slot] = mem;
    if (mem == NULL) return -1;
    memset(mem, 0, lbl_805FBB90);
    u8* aligned = (u8*)(((uintptr_t)lbl_805FBB88[slot] + 0x1F) & ~(uintptr_t)0x1F);
    *(u8**)(lbl_8051DAD0 + slot * 4 + 0x94) = aligned;
    lbl_805FBB80[slot] = aligned;
    *(u16*)(lbl_8051DAD0 + 0x78) = 0;
    *(s32*)(lbl_8051DAD0 + 0x7C) = 0;
    if (lbl_805F98C1 != 0) {
        *(s32*)(lbl_8051DAD0 + 0x80) = *(s32*)(lbl_8051DAD0 + 0x34);
    } else {
        *(s32*)(lbl_8051DAD0 + 0x80) = *(s32*)(file + 0x7C);
    }
    *(s32*)(lbl_8051DAD0 + 0x84) = 0;
    s32 length = *(s32*)(lbl_8051DAD0 + 0x80);
    if (length > 0x10000) length = 0x10000;
    u32 head = (u32)length & 0xFFFFFFE0u;          // bytes read into the buffer, 32-byte aligned
    u32 rest = 0x10000 - head;
    u8* dst = *(u8**)(lbl_8051DAD0 + slot * 4 + 0x94);
    memset(dst + head, 0, rest);
    DCFlushRange(dst + head, rest);
    if (lbl_805F98C1 != 0) {
        if (slot == 1) {
            *(s32*)(lbl_8051DAD0 + 0xB4) = 7;
        }
    } else {
        fn_80146930(file + 0x1C, *(u8**)(lbl_8051DAD0 + slot * 4 + 0x94), (s32)head);
    }
    return 0;
}

// 0x802049E0
// Plays the Bink movie 'path' full screen (player+0 is the volume 0..1). 'subtitleName' (may be NULL)
// is a base name whose ".txt" is loaded as subtitles. Any 'button' (or -1 for all controllers)
// skips it after 3 seconds unless 'duration' is non-zero; 'moreQueued' keeps the screen as is for
// the next movie; 'stopCb' is polled every frame and aborts when it returns non-zero. Retries up to
// 4 times if the file fails to open or read. Always returns 0.
s32 MoviePlay(u8* player, const char* path, const char* subtitleName, u32 unused, u8 moreQueued,
              s32 duration, s32 button, BOOL (*stopCb)(s32)) {
    (void)unused;
    char subPath[0x80];
    u8 file[0x60];
    u8 readBuf[4];
    u8 color[4];
    s32 soundTrack;

    fn_80188ED4(2);
    fn_80188E5C(NULL);
    s32 subtitles = 0;
    lbl_805FB630 = 0xFF64FFFF;
    if (subtitleName != NULL) {
        strcpy(subPath, subtitleName);
        strcat(subPath, ".txt");
        subtitles = fn_801C63E4(subPath);
    }
    s32 buttonHeld = 0x108B + button * 0x1000;       // fn_80147554 code used while the movie starts
    s32 buttonPressed = button * 0x10 + 4;           // code used for the skip check
    s32 attempt = 0;
    s32 retry = 1;
    s32 startFrame = 1;
    u32 aborted = 0;
    while (retry != 0) {
        if (duration != 0) startFrame = 1;
        retry = 0;
        // Soundtrack index from the text language.
        if (gTextLanguage == 0) soundTrack = 0;
        else if (gTextLanguage == 1) soundTrack = 1;
        else if (gTextLanguage == 3) soundTrack = 2;
        else if (gTextLanguage == 2) soundTrack = 3;
        else soundTrack = (gTextLanguage == 4) ? 4 : 0;
        fn_802B47A0(1, &soundTrack);
        lbl_805FBB18 = fn_802B4B30(path, 0x104400);
        if (lbl_805FBB18 == NULL) retry = 1;
        if (lbl_805FBB18 != NULL) {
            fn_802B6C60(lbl_805FBB18, 0, (s32)(32768.0f * *(f32*)player));
            s32 hadVideo = (lbl_805FB51C != 0);
            if (lbl_805FB51C != 0) {
                RendClear(5, 0, 0, 1.0f);
                fn_80150148();
                fn_801FC0A0();
                fn_801FC1EC();
                RendEndFrame();
            }
            fn_8015D530();
            fn_802B4A1C(lbl_805FBB18, lbl_80526F28 + 0x100);
            if (fn_80205454(lbl_80526F28) != 0) {
                fn_802B4B14(lbl_805FBB18, lbl_80526F28 + 0x100);
                fn_802B6574(lbl_805FBB18, startFrame, 0);
                aborted = 0;
                for (;;) {
                    // Decode until the frame is ready.
                    fn_80206308(lbl_80526F28);
                    fn_802B5C6C(lbl_805FBB18);
                    bool failed = false;
                    while (fn_802B61D0(lbl_805FBB18) != 0) {
                        fn_802B625C(lbl_805FBB18);
                        if (*(s32*)(lbl_805FBB18 + 0x1C) != 0) {
                            startFrame = *(s32*)(lbl_805FBB18 + 0xC);
                            retry = 1;
                            failed = true;
                            break;
                        }
                        fn_802B5C6C(lbl_805FBB18);
                    }
                    if (failed) break;
                    fn_802062F0(lbl_80526F28);
                    fn_8015D578();
                    s32 movieW = *(s32*)(lbl_805FBB18 + 0x0);
                    s32 movieH = *(s32*)(lbl_805FBB18 + 0x4);
                    s32 screenW = VideoGetWidth(&gVideo);
                    s32 screenH = VideoGetHeight(&gVideo);
                    fn_80205894(lbl_80526F28, (f32)((screenW - movieW) / 2), (f32)((screenH - movieH) / 2),
                                1.0f, 1.0f, 1.0f, movieW, movieH, 1);
                    if (subtitles != 0) fn_80204838(lbl_805FBB18);
                    BOOL held = 0;
                    if (button == -1) {
                        for (s32 i = 0; i < 4; i++) {
                            if (fn_80147554(0x108B + i * 0x1000)) held = 1;
                        }
                    } else {
                        if (fn_80147554(buttonHeld)) held = 1;
                    }
                    if (held != 0) {
                        fn_801FCA38((f32)gScreenWidth, (f32)gScreenHeight);
                    }
                    fn_801FC0A0();
                    fn_801FC70C();
                    color[0] = 0;
                    color[1] = 0;
                    color[2] = 0;
                    color[3] = 0;
                    GXSetCopyClear(color, 0xFFFFFF);
                    fn_8015D5A4();
                    fn_80142970();
                    s32 done = 0;
                    // Present frames until the movie ends or is skipped.
                    do {
                        u32 rateNum = *(u32*)(lbl_805FBB18 + 0x14);
                        u32 rateDen = *(u32*)(lbl_805FBB18 + 0x18);
                        s32 frameNo = *(s32*)(lbl_805FBB18 + 0x10);
                        f32 fps = (f32)rateNum / (f32)rateDen;
                        f32 seconds = (f32)frameNo / fps;
                        BOOL skip = 0;
                        if (seconds > 3.0f) {
                            if (button == -1) {
                                for (s32 i = 0; i < 4; i++) {
                                    if (fn_80147554(4 + i * 0x10)) { skip = 1; break; }
                                }
                            } else {
                                skip = fn_80147554(buttonPressed) != 0;
                            }
                        }
                        if (skip != 0 && duration == 0) {
                            done = 1;
                            break;
                        }
                        if (stopCb != NULL && stopCb(0) != 0) {
                            done = 1;
                            break;
                        }
                        if (SysGetExitMode() != 0) {
                            done = 1;
                            aborted = 1;
                            break;
                        }
                    } while (fn_802B6950(lbl_805FBB18) != 0);
                    if (done != 0) break;
                    if (*(u32*)(lbl_805FBB18 + 0xC) == *(u32*)(lbl_805FBB18 + 0x8)) break;
                    fn_802B625C(lbl_805FBB18);
                    if (*(s32*)(lbl_805FBB18 + 0x1C) != 0) {
                        retry = 1;
                        startFrame = *(s32*)(lbl_805FBB18 + 0xC);
                        break;
                    }
                }
            }
            fn_8020506C(lbl_80526F28);
            fn_8015DDEC();
            if (hadVideo != 0) {
                SetRenderEnabled(0);
                if (moreQueued == 0 || aborted == 1) {
                    for (s32 i = 0; i < 2; i++) {
                        RendBeginFrame();
                        RendClear(7, 0, 0, 1.0f);
                        fn_80150148();
                        fn_801FC0A0();
                        fn_801FC1EC();
                        RendEndFrame();
                    }
                    fn_80150DEC();
                    fn_80151014();
                    fn_80151244();
                }
                RendBeginFrame();
                if (aborted == 0) SetRenderEnabled(1);
            }
            fn_802B6798(lbl_805FBB18);
            lbl_805FBB18 = NULL;
        }
        if (retry != 0) {
            // The movie failed: check the file is readable (disc problem), else call the error callback.
            if (fn_80146808(file, path, "rb") != 0) {
                fn_80146930(file, readBuf, 1);
                fn_801469FC(file);
            } else {
                void* cb = *(void**)lbl_805FB470;
                if (cb != NULL) ((void (*)(s32))cb)(0);
            }
        }
        attempt++;
        if (attempt > 3 && duration == 0) break;
    }
    if (subtitles != 0) fn_801C64E8();
    fn_80188ED4(0);
    fn_80188E5C(NULL);
    return 0;
}

}
