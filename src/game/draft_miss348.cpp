// Drafts miss348: audio voice slots, DSP stream refill and the movie subtitle/overlay draw
// (0x801FC70C..0x80204838).
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "revolution.h"

extern "C" {
s32   DVDReadAsync(u8* fileInfo, void* addr, s32 len, s32 offset, void* cb, s32 prio);
void  GameLog(int level, int category, const char* fmt, ...);
void  OSCreateAlarm(u8* alarm);                                   // 0x802340C4
s32   fn_80146930(u8* file, void* buf, s32 size);                  // 0x80146930
s32   fn_80146A58(u8* file);                                       // 0x80146A58
void  fn_8015D660(void);                                           // 0x8015D660
void  fn_80188E5C(u8* p);                                          // 0x80188E5C
void  fn_80188EDC(const char* text, s32* w, s32* h);               // 0x80188EDC (text size)
void  fn_80188F00(const char* fmt, ...);                           // 0x80188F00
void* fn_801B7198(void* anim, s32 a);                              // 0x801B7198
s32   fn_801C6564(f32 v);                                          // 0x801C6564
s32   fn_801C6604(s32 a3);                                         // 0x801C6604
const char* fn_801C6618(s32 a, s32 b);                             // 0x801C6618
void  fn_801FF910(f32 v);                                          // 0x801FF910
void  fn_801FFC58(f32 v);                                          // 0x801FFC58
void  fn_80201B94(s32 result, u8* fileInfo);                       // 0x80201B94 (DVD read callback)
void  fn_80201BD4(u8* audio, u8* state);                           // 0x80201BD4
void  fn_80202CAC(void);                                           // 0x80202CAC (alarm callback)
void  fn_80202EFC(s32 chan, s32 result);                           // 0x80202EFC (pad connect result callback)
void  fn_80203ABC(u8* audio, u8* state);                           // 0x80203ABC
void  fn_80203DE8(u8* audio, u8* state);                           // 0x80203DE8
void fn_802044C0(u8* tex, u32 color, f32 x0, f32 x1, f32 y0, f32 y1, f32 u0, f32 u1, f32 v0, f32 v1);                                           // 0x802044C0
s32   fn_802097CC(u8* obj, f32* a, f32* b, f32* c, f32 t);         // 0x802097CC (vcall slot 0x54)
void  OSSetPeriodicAlarm(u8* alarm, u32 a, OSTime start, OSTime period, void* handler, u32 extra); // 0x80234394
void  C_MTXOrtho(f32* mtx, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);  // 0x8024849C
void  GXSetVtxDesc(s32 a, s32 b);                                   // 0x8024A3A4
void  GXClearVtxDesc(void);                                           // 0x8024A7E4
void  GXSetVtxAttrFmt(s32 a, s32 b, s32 c, s32 d, s32 e);             // 0x8024A818
void  GXSetTexCoordGen2(s32 coord, s32 func, s32 src, s32 mtx, s32 normalize, s32 postMtx);  // 0x8024AC4C
void  GXSetNumTexGens(s32 a);                                          // 0x8024AE74
void  GXBegin(s32 a, s32 b, s32 c);                            // 0x8024B974
void  fn_8024BDD4(u32 aa, u8* pattern, s32 vf, u8* vfilter);       // 0x8024BDD4: GXSetCopyFilter
void  GXSetNumChans(s32 a);                                          // 0x8024C2F0
void  GXSetChanCtrl(s32 chan, s32 enable, s32 ambSrc, s32 matSrc, s32 lightMask, s32 diffFn, s32 attnFn);  // 0x8024C314
void  GXInitTexObjCI(u8* obj, void* image, s32 w, s32 h, s32 fmt, s32 wrapS, s32 wrapT, s32 mip, s32 extra);  // 0x8024C5C4 (9th arg on the stack)
void  GXLoadTexObj(u8* obj, s32 id);                                // 0x8024C91C
void  fn_8024C970(u8* obj, void* lut, s32 fmt, s32 n);             // 0x8024C970
void  fn_8024C998(u8* obj, s32 id);                                // 0x8024C998
void  fn_8024D59C(s32 a, s32 b, s32 c, s32 d, s32 e);             // 0x8024D59C
void  fn_8024D5DC(s32 a, s32 b, s32 c, s32 d, s32 e);             // 0x8024D5DC
void  fn_8024D61C(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f);      // 0x8024D61C
void  fn_8024D674(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f);      // 0x8024D674
void  fn_8024D6CC(s32 id, u32* color);                             // 0x8024D6CC (reads the packed RGBA word)
void  fn_8024DA08(s32 a, s32 b, s32 c, s32 d);                    // 0x8024DA08
void  fn_8024DB64(s32 a);                                          // 0x8024DB64
void  GXSetBlendMode(s32 a, s32 b, s32 c, s32 d);                    // 0x8024DED8
void  GXSetZMode(s32 a, s32 b, s32 c);                            // 0x8024DF80
void  GXSetProjection(f32* mtx, s32 a);                                // 0x8024E1D0
void  GXSetCurrentMtx(s32 a);                                          // 0x8024E328
s32   DVDOpen(char* path, u8* p);                              // 0x8024F1E8
void  fn_80264420(u8* p);                                          // 0x80264420
s32   fn_80264A24(u8* list, u8* item);                             // 0x80264A24 (unlink item from list)
s32   WPADProbe(s32 chan, u32* out);                             // 0x802728A4
s32   WPADControlSpeaker(s32 chan, s32 a, void* cb);                      // 0x802737A8
f64   fn_8033F204(f64 x);                                          // 0x8033F204 (natural log)
f64   fn_8033F20C(f64 x, f64 y);                                   // 0x8033F20C (pow)

extern u8  gAudio[];
extern s32 gScreenHeight;
extern s32 gScreenWidth;
extern u8  lbl_803865F0[];          // 0x803865F0 (string table)
extern MEMAllocator lbl_803F2870;   // 0x803F2870
extern u8  lbl_80412BE0[];          // 0x80412BE0 (static record)
extern f32 lbl_804FDA40[];          // 0x804FDA40 (quad vertex data)
extern u8  lbl_8051DAD0[];          // 0x8051DAD0 (audio stream state)
extern u8  lbl_8051E600[];          // 0x8051E600 (alarm)
extern u8  lbl_8051E680[];          // 0x8051E680
extern u8  lbl_8051E6A8[];          // 0x8051E6A8 (voice slot table, 0x114 bytes each)
extern const char lbl_805F98E4[];   // 0x805F98E4 (".dsp")
extern const char lbl_805F98F8[];   // 0x805F98F8 ("%s")
extern u8  lbl_805F98C0;            // 0x805F98C0
extern u8  lbl_805F98C1;            // 0x805F98C1
extern f32 lbl_805F98C4;            // 0x805F98C4
extern f32 lbl_805F98C8;            // 0x805F98C8
extern f32 lbl_805F98CC;            // 0x805F98CC
extern f32 lbl_805F98D4;            // 0x805F98D4
extern f32 lbl_805F996C;            // 0x805F996C
extern u8  lbl_80600300;            // 0x80600300
extern u8  lbl_80600301;            // 0x80600301
extern u8  lbl_80600302;            // 0x80600302
extern s32 lbl_805FB628;            // 0x805FB628
extern s32 lbl_805FB62C;            // 0x805FB62C
extern s32 lbl_805FB63C;            // 0x805FB63C
extern s32 lbl_805FB6B8;            // 0x805FB6B8
extern u8* lbl_805FBA78;            // 0x805FBA78
extern u8  lbl_805FBA7C;            // 0x805FBA7C
extern u8  lbl_805FBAA4;            // 0x805FBAA4
extern s32 lbl_805FBAA8;            // 0x805FBAA8
extern s32 lbl_805FBAAC;            // 0x805FBAAC
extern u8  lbl_805FBAB8;            // 0x805FBAB8
extern s32 lbl_805FBABC;            // 0x805FBABC
extern u8  lbl_805FBAC4;            // 0x805FBAC4
extern u8  lbl_805FBAD5;            // 0x805FBAD5
extern s32 lbl_805FBAE0;            // 0x805FBAE0 (critical-section flag)
extern f32 lbl_805FBAE8;            // 0x805FBAE8
extern f32 lbl_805FBAEC;            // 0x805FBAEC
extern f32 lbl_805FBAF0;            // 0x805FBAF0
extern f32 lbl_805FBAF4;            // 0x805FBAF4
extern s32 lbl_805FBAF8;            // 0x805FBAF8
extern s32 lbl_805FBAFC;            // 0x805FBAFC
extern u8  lbl_805FBB78;            // 0x805FBB78
extern u8* lbl_805FBB94;            // 0x805FBB94
extern u8* lbl_805FBB98;            // 0x805FBB98
extern u32 lbl_805FBB9C;            // 0x805FBB9C
}

// Voice slot i of the table (stride 0x114): the voice object pointer at +0xC.
static inline u8* SlotVoice(s32 i) {
    return *(u8**)(lbl_8051E6A8 + i * 0x114 + 0xC);
}

// Moves from `from` toward `to` by u (0..1 over the fade) and clamps to the range between them.
static f32 FadeLerp(f32 from, f32 to, f32 u) {
    f32 v = from + u * (to - from);
    if (from < to) {
        if (v < from) return from;
        if (v > to) return to;
        return v;
    }
    if (v < to) return to;
    if (v > from) return from;
    return v;
}

// Clamps to [0, 1].
static f32 Clamp01(f32 v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// Publishes the fade volume and rescales the volume (+0x28) of every type 6/7 voice from its
// base volume (+0x24).
static void ApplyFadeVolume(f32 vol) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    lbl_805F98D4 = vol;
    for (s32 i = 2; i < 96; i++) {
        u8* v = SlotVoice(i);
        if (v != NULL && (u32)(*(s32*)(v + 0x14) - 6) <= 1u) {
            *(s32*)(v + 0x28) = (s32)((f32)*(s32*)(v + 0x24) * vol);
        }
    }
    lbl_805FBAE0 = saved;
}

// Frees a voice record: unlinks it from its parent slot, clears the animation back-reference and
// resets every field to "unused" (-1 / 0).
static void ReleaseVoice(u8* v, s32 saved) {
    if (v == NULL || *(s32*)(v + 0x0) == -1) {
        lbl_805FBAE0 = saved;
        return;
    }
    s32 parent = *(s32*)(v + 0x8);
    if (parent != -1) {
        *(u8**)(lbl_8051E6A8 + parent * 0x114 + 0xC) = NULL;
    }
    s32 sp8 = *(s32*)(v + 0x44);
    u8* anim = (u8*)fn_801B7198(&sp8, 0);
    if (anim != NULL && *(s32*)(anim + 0xAC) == *(s32*)(v + 0x0)) {
        *(s32*)(anim + 0xAC) = -1;
    }
    *(s32*)(v + 0x0) = -1;
    *(s32*)(v + 0x4) = -1;
    *(s32*)(v + 0x8) = -1;
    *(s32*)(v + 0xC) = 0;
    *(s32*)(v + 0x10) = 0;
    *(s32*)(v + 0x14) = -1;
    *(s32*)(v + 0x44) = lbl_805FB6B8;
    *(s32*)(v + 0x4C) = 0;
    *(s32*)(v + 0x50) = 0;
    *(s32*)(v + 0x30) = 0;
    *(f32*)(v + 0x34) = 0.0f;
    *(s32*)(v + 0x54) = 0;
    lbl_805FBAE0 = saved;
}

extern "C" {

// 0x801FC70C
// Draws the full-screen overlay quad (movie/fade backdrop): ortho setup, TEV/blend state, a
// palette texture and one textured quad through the write-gather pipe.
void fn_801FC70C(void) {
    f32 sp48[16];
    u8 sp28[0x20];
    u8 sp18[0x10];
    u32 sp14;
    u8 sp10[4];

    if (lbl_805FBA7C == 0) return;
    fn_8015D660();
    fn_8024BDD4(0, NULL, 0, NULL);
    C_MTXOrtho(sp48, 0.0f, (f32)gScreenHeight, 0.0f, (f32)gScreenWidth, 0.0f, 500.0f);
    GXSetProjection(sp48, 1);
    GXClearVtxDesc();
    GXSetVtxAttrFmt(5, 9, 0, 3, 0);
    GXSetVtxAttrFmt(5, 0xD, 1, 3, 0);
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xD, 1);
    GXSetNumChans(1);
    GXSetChanCtrl(4, 0, 1, 1, 0, 0, 2);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(0, 1, 4, 0x3C, 0, 0x7D);
    fn_8024DB64(1);
    sp10[0] = lbl_80600300;
    sp10[1] = lbl_80600301;
    sp10[2] = lbl_80600302;
    sp10[3] = lbl_805FBA7C;
    sp14 = *(u32*)sp10;
    fn_8024D6CC(1, &sp14);
    fn_8024DA08(0, 0, 0, 0xFF);
    fn_8024D59C(0, 0xF, 0xF, 0xF, 8);
    fn_8024D61C(0, 0, 0, 0, 1, 0);
    fn_8024D5DC(0, 7, 1, 4, 7);
    fn_8024D674(0, 0, 0, 0, 1, 0);
    GXSetBlendMode(1, 4, 5, 0);
    GXSetZMode(0, 3, 0);
    GXSetCurrentMtx(3);
    u8* t = lbl_805FBA78;
    fn_8024C970(sp18, *(void**)(t + 0x20), *(u8*)(t + 0x15), 0x10);
    fn_8024C998(sp18, 0);
    GXInitTexObjCI(sp28, *(void**)(t + 0x1C), *(u16*)(t + 0x4), *(u16*)(t + 0x6), *(u8*)(t + 0x14), 0, 0, 0, 0);
    GXLoadTexObj(sp28, 0);
    GXBegin(0x80, 4, 4);
    GXWGFifo._f32 = lbl_804FDA40[0];
    GXWGFifo._f32 = lbl_804FDA40[1];
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = lbl_804FDA40[2];
    GXWGFifo._f32 = lbl_804FDA40[3];
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = lbl_804FDA40[4];
    GXWGFifo._f32 = lbl_804FDA40[5];
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._f32 = 1.0f;
    GXWGFifo._f32 = lbl_804FDA40[6];
    GXWGFifo._f32 = lbl_804FDA40[7];
    GXWGFifo._f32 = 0.0f;
    GXWGFifo._f32 = 1.0f;
}

// 0x801FD95C
// Per-frame audio update: runs the 4-state music duck fade (0 idle, 1 fading down, 2 held,
// 3 fading back), kicks the stream refill for the two DSP streams and recounts the active voices.
void fn_801FD95C(u8* arg0, f32 farg0, f32 farg1) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;

    // Any voice of type 2 active?
    s32 active = 0;
    for (s32 i = 2; i < 96; i++) {
        u8* v = SlotVoice(i);
        if (v != NULL && *(s32*)(v + 0x14) == 2) active = 1;
    }

    switch (lbl_805FBAF8) {
    case 0:
        if (active != 0) {
            lbl_805FBAEC = lbl_805F98C8;
            lbl_805FBAF0 = lbl_805F98C4;
            lbl_805FBAF4 = lbl_805F98D4;
            lbl_805FBAE8 = 0.5f;
            lbl_805FBAF8 = 1;
        }
        break;
    case 1: {
        f32 a = lbl_805FBAEC;
        f32 k = lbl_805F996C;
        f32 t = lbl_805FBAE8 - farg1;
        lbl_805FBAE8 = t;
        f32 u = (t - 0.5f) / -0.5f;
        f32 f1 = FadeLerp(a, a * k, u);
        f32 b = lbl_805FBAF0;
        f32 f30 = FadeLerp(b, b * lbl_805F996C, u);
        f32 c = lbl_805FBAF4;
        f32 f31 = FadeLerp(c, c * lbl_805F996C, u);
        fn_801FFC58(f1);
        fn_801FF910(f30);
        ApplyFadeVolume(Clamp01(f31));
        if (lbl_805FBAE8 <= 0.0f) lbl_805FBAF8 = 2;
        if (active == 0) {
            f32 v = 1.5f * ((lbl_805FBAE8 - 0.5f) / -0.5f);
            if (v < 0.0f) v = 0.0f;
            else if (v > 1.5f) v = 1.5f;
            lbl_805FBAE8 = v;
            lbl_805FBAF8 = 3;
        }
        break;
    }
    case 2:
        if (active == 0) {
            lbl_805FBAF8 = 3;
            lbl_805FBAE8 = 1.5f;
        }
        break;
    case 3: {
        f32 a = lbl_805FBAEC;
        f32 t = lbl_805FBAE8 - farg1;
        lbl_805FBAE8 = t;
        f32 u = (t - 1.5f) / -1.5f;
        f32 f1 = FadeLerp(a * lbl_805F996C, a, u);
        f32 b = lbl_805FBAF0;
        f32 f30 = FadeLerp(b * lbl_805F996C, b, u);
        f32 c = lbl_805FBAF4;
        f32 f31 = FadeLerp(c * lbl_805F996C, c, u);
        fn_801FFC58(f1);
        fn_801FF910(f30);
        ApplyFadeVolume(Clamp01(f31));
        if (lbl_805FBAE8 <= 0.0f) lbl_805FBAF8 = 0;
        if (active != 0) {
            f32 v = 0.5f * ((lbl_805FBAE8 - 1.5f) / -1.5f);
            if (v < 0.0f) v = 0.0f;
            else if (v > 0.5f) v = 0.5f;
            lbl_805FBAE8 = v;
            lbl_805FBAF8 = 1;
        }
        break;
    }
    default:
        break;
    }

    if ((s8)*(u8*)(arg0 + 0x29) == 0 && (s8)*(u8*)(arg0 + 0x28) != 0) {
        s32 ready;
        if (lbl_805F98C0 != 0) {
            if (lbl_805F98C1 != 0) {
                if (*(s32*)(lbl_8051DAD0 + 0xBC) != 0) {
                    ready = 0;
                    if (*(s32*)(lbl_8051DAD0 + 0x34) != 0 && *(s32*)(lbl_8051DAD0 + 0x70) != 0) ready = 1;
                } else {
                    ready = 0;
                }
            } else {
                ready = 0;
                if (fn_80146A58(arg0 + 0x3C) != 0 && fn_80146A58(arg0 + 0x1E4) != 0) ready = 1;
            }
        } else {
            ready = 0;
        }
        if (ready != 0) fn_80203ABC(arg0, lbl_8051DAD0);
    }
    if (lbl_805FBAA8 == 0 && lbl_805FBAAC != 0) {
        s32 ready;
        if (lbl_805FBAA4 != 0 && *(s32*)(lbl_8051DAD0 + 0x17C) != 0) {
            ready = *(s32*)(lbl_8051DAD0 + 0xF4) != 0;
        } else {
            ready = 0;
        }
        if (ready != 0) fn_80203ABC(arg0, lbl_8051DAD0 + 0xC0);
    }

    *(s32*)(arg0 + 0x2C0) = 0;
    *(s32*)(arg0 + 0x2BC) = 0;
    if (lbl_805FBB78 != 0 || lbl_805FBAD5 != 0) {
        u32 n = (u32)lbl_805FBAFC + 1u;
        lbl_805FBAFC = (s32)n;
        if (n % 40u == 0) lbl_805FBAFC = 0;
    }
    // Count the live voices and track the highest value of field +0x30.
    for (s32 i = 2; i < 96; i++) {
        u8* v = SlotVoice(i);
        if (v != NULL) {
            *(s32*)(arg0 + 0x2C0) = *(s32*)(arg0 + 0x2C0) + 1;
            s32 w = *(s32*)(v + 0x30);
            if (*(s32*)(arg0 + 0x2BC) < w) *(s32*)(arg0 + 0x2BC) = w;
        }
    }
    lbl_805FBAE0 = saved;
}

// 0x801FE308
// Finds a free voice slot for a new voice of the kind in *(arg0 + 0x14): kind 1 takes the slot
// at which more than 6 free ones were counted, other kinds the first free slot; kind 2 with no
// free slot evicts the first non-type-2 voice. Returns the slot index or -1.
s32 fn_801FE308(u8* arg0) {
    s32 kind = *(s32*)(arg0 + 0x14);
    s32 r3 = -1;
    if (kind == 1) {
        s32 freeCount = 0;
        for (s32 i = 2; i < 96; i++) {
            if (SlotVoice(i) == NULL) {
                freeCount += 1;
                if (freeCount > 6) {
                    r3 = i;
                    break;
                }
            }
        }
    } else {
        for (s32 i = 2; i < 96; i++) {
            if (SlotVoice(i) == NULL) {
                r3 = i;
                break;
            }
        }
    }
    if (r3 == -1 && kind == 2) {
        for (s32 i = 2; i < 96; i++) {
            u8* v = SlotVoice(i);
            if (v == NULL || *(s32*)(v + 0x14) != 2) {
                s32 saved = lbl_805FBAE0;
                u8* victim = SlotVoice(i);
                lbl_805FBAE0 = 1;
                ReleaseVoice(victim, saved);
                return i;
            }
        }
    }
    return r3;
}

// 0x8020151C
// Per-frame update of one voice: derives the two fixed-point dB gains (+0xC0, +0x70 of its slot)
// from the pan/volume fields, fades the voice out and frees its list entries once quiet.
// Returns 1 when the voice is finished.
s32 fn_8020151C(u8* arg0) {
    f32 sp10, spC, sp8;
    if (arg0 == NULL) return 0;
    if (*(s32*)(arg0 + 0x8) == -1) return 1;

    sp8 = *(f32*)(arg0 + 0x38);
    sp10 = 0.5f;
    spC = 0.5f;
    s32 r31 = fn_802097CC(arg0, &sp10, &spC, &sp8, *(f32*)(arg0 + 0x38));
    u8* slot = lbl_8051E6A8 + *(s32*)(arg0 + 0x8) * 0x114;
    if (*(u8*)(slot + 0xFC) != 0) {
        f32 f30 = (f32)fn_8033F20C(10.0, (f64)(((f32)(*(s32*)(slot + 0x70) >> 16) / 10.0f) / 30.0f));
        if (f30 > -0.0001f && f30 < 0.0001f) f30 = 0.0f;
        f32 f1 = (f32)fn_8033F20C(10.0, (f64)(((f32)(*(s32*)(slot + 0x4C) >> 16) / 10.0f) / 30.0f));
        if (f1 > -0.0001f && f1 < 0.0001f) f1 = 0.0f;
        f32 f28 = f30 * f1;
        if (*(u8*)(slot + 0x105) != 0) {
            f32 f31 = f28 * lbl_805F98CC;
            f32 db;
            if (f31 > -0.0001f && f31 < 0.0001f) {
                db = -1000.0f;
            } else {
                f32 ln2 = (f32)fn_8033F204(2.0);
                f32 q = (f32)fn_8033F204(10.0) / ln2;
                f32 ln2b = (f32)fn_8033F204(2.0);
                db = (30.0f * ((f32)fn_8033F204((f64)f31) / ln2b)) / q;
            }
            s32 g = (s32)(10.0f * db);
            if (g < -0x8000) g = -0x8000;
            else if (g > 0x7FFF) g = 0x7FFF;
            *(s32*)(slot + 0xC0) = g << 16;
            if (f28 < 0.1f) {
                s32 saved = lbl_805FBAE0;
                lbl_805FBAE0 = 1;
                fn_80264A24(slot + 0x10, slot + 0xB8);
                fn_80264A24(slot + 0x10, slot + 0xAC);
                *(u8*)(slot + 0xFC) = 0;
                if (*(u8*)(slot + 0xFD) != 0) {
                    s32 n = lbl_805FBABC - 1;
                    lbl_805FBABC = n;
                    if (n < 0) lbl_805FBABC = 0;
                    *(u8*)(slot + 0xFD) = 0;
                }
                lbl_805FBAE0 = saved;
            } else if (*(u8*)(slot + 0xFD) == 0) {
                *(u8*)(slot + 0xFD) = 1;
                lbl_805FBABC += 1;
            }
        }
    }
    *(u8*)(slot + 0x105) = (u8)(*(u8*)(slot + 0x105) + 1);
    if (r31 == 1) return 1;

    f32 s = sp8;
    *(f32*)(arg0 + 0x38) = s;
    *(s32*)(lbl_8051E6A8 + *(s32*)(arg0 + 0x8) * 0x114 + 0x64) = (s32)(1200.0f * (10.0f * (s - 1.0f))) << 16;
    f32 vol = (f32)*(s32*)(arg0 + 0x28);
    f32 f28 = vol / 32768.0f;
    *(f32*)(arg0 + 0x2C) = vol;
    if (!(*(s32*)(arg0 + 0x50) & 0x200)) {
        s32 kind = *(s32*)(arg0 + 0x14);
        if (kind != 3 && kind != 4 && kind != 5 && f28 < 0.01f) return 1;
    }
    f32 db;
    if (f28 > -0.0001f && f28 < 0.0001f) {
        db = -1000.0f;
    } else {
        f32 ln2 = (f32)fn_8033F204(2.0);
        f32 q = (f32)fn_8033F204(10.0) / ln2;
        f32 ln2b = (f32)fn_8033F204(2.0);
        db = (30.0f * ((f32)fn_8033F204((f64)f28) / ln2b)) / q;
    }
    s32 g = (s32)(10.0f * db);
    if (g < -0x8000) g = -0x8000;
    else if (g > 0x7FFF) g = 0x7FFF;
    *(s32*)(lbl_8051E6A8 + *(s32*)(arg0 + 0x8) * 0x114 + 0x70) = g << 16;
    return 0;
}

// 0x80201D9C
// Refills the next block of a DSP stream buffer: zero-fills the unread tail (up to 0x8000
// bytes), reads the next chunk (async from disc or through the file layer) and advances the
// stream bookkeeping.
void fn_80201D9C(u8* arg0, u8* arg1) {
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    if (arg1 == NULL) {
        lbl_805FBAE0 = saved;
        return;
    }
    u8* base = *(u8**)(arg1 + 0x94);
    if (base == NULL) {
        *(s32*)(arg1 + 0xB8) = 0;
        lbl_805FBAE0 = saved;
        return;
    }
    s32 remain = *(s32*)(arg1 + 0x80);
    u8* buf = base + *(s32*)(arg1 + 0xA4);
    *(s32*)(arg1 + 0xBC) = 0;
    *(s32*)(arg1 + 0xB8) = 1;
    s32 chunk = 0x8000;
    if (remain <= 0x8000) chunk = remain;
    chunk &= ~0x1F;
    u32 rest = (u32)(0x8000 - chunk);
    u8* tail = buf + chunk;
    memset(tail, 0, rest);
    DCFlushRange(tail, rest);
    if (lbl_805F98C1 != 0) {
        DVDReadAsync(arg1, buf, chunk, *(s32*)(arg1 + 0x84), (void*)fn_80201B94, 2);
    } else {
        fn_80146930(arg0 + 0x3C, buf, chunk);
        DCFlushRange(buf, (u32)chunk);
        u8* st = lbl_8051DAD0;
        if (st == NULL) {
            fn_80201BD4(gAudio, st);
        } else {
            st += 0xC0;
            if (st == NULL) fn_80201BD4(gAudio, st);
        }
    }
    if (*(s32*)(arg1 + 0xA0) == 1) {
        *(s32*)(arg1 + 0x80) = *(s32*)(arg1 + 0x80) - chunk;
        *(s32*)(arg1 + 0x84) = *(s32*)(arg1 + 0x84) + chunk;
    }
    if (((*(s32*)(arg1 + 0x80) - chunk) & ~0x1F) == 0) {
        if (chunk != 0) {
            // physical (uncached-to-physical) address of the end of the chunk
            *(u8**)(arg1 + 0x88) = (u8*)((uintptr_t)(buf + chunk) + 0x80000000u);
        }
        *(s32*)(arg1 + 0x7C) = *(s32*)(arg1 + 0x7C) + 1;
    }
    lbl_805FBAE0 = saved;
}

// 0x80202B8C
// Voice callback: clears the voice's active flag, stops its list entry and frees its voice
// record.
void fn_80202B8C(u8* arg0) {
    u8* v = *(u8**)(arg0 + 0x14);
    if (v == NULL) return;
    *(s32*)(v + 0x0) = 0;
    if (*(u8*)(v + 0xFE) != 0) {
        fn_80264420(v + 0x10);
    }
    u8* parent = *(u8**)(v + 0xC);
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    ReleaseVoice(parent, saved);
}

// 0x80203220
// Polls the pad connection: on a fresh connect starts the callback, and (once) creates the
// periodic audio-tick alarm; otherwise flags a pad error.
void fn_80203220(void) {
    s32 err = 0;
    s32 r = WPADProbe(0, NULL);
    if (r == 0) {
        if (*(s32*)(lbl_8051E680 + 0x24) == 6) {
            *(s32*)(lbl_8051E680 + 0x24) = 0;
            *(s32*)(lbl_8051E680 + 0x24) = 1;
            WPADControlSpeaker(0, 1, (void*)fn_80202EFC);
        }
    } else if (r != -1) {
        err = 1;
    }
    if (err == 0) {
        if (lbl_805FBAB8 == 0) {
            OSCreateAlarm(lbl_8051E600);
            OSTime now = OSGetTime();
            u32 timer = OSGetBusClock() >> 2;
            u32 ms = (u32)(((u64)0x431BDE83u * timer) >> 32) >> 15;
            u32 period = (u32)(((u64)0x10624DD3u * (u32)(ms * 0x65B9ABu)) >> 32) >> 9;
            OSSetPeriodicAlarm(lbl_8051E600, ms, now, (OSTime)period, (void*)fn_80202CAC, 0x10624DD3u);
            lbl_805FBAB8 = 1;
        }
    } else {
        lbl_805FBAC4 = 1;
    }
}

// 0x8020390C
// Opens the DSP file "<prefix><name>.dsp" and (re)allocates the 0x10020-byte stream buffer,
// aligned to 32 bytes, zero-filling the unread tail. Returns 1, or -1 on failure.
s32 fn_8020390C(u8* arg0, u8* arg1) {
    char buf[0x80];
    strcpy(buf, (const char*)(lbl_803865F0 + 0x5B8));
    strcat(buf, (const char*)arg1);
    strcat(buf, lbl_805F98E4);
    if (lbl_805FBAA4 != 0 && *(s32*)(lbl_8051DAD0 + 0x174) != 0) {
        fn_80203DE8(arg0, lbl_8051DAD0 + 0xC0);
        while (*(volatile s32*)(lbl_8051DAD0 + 0x174) != 0) {
        }
    }
    GameLog(1, 2, (const char*)(lbl_803865F0 + 0x5C8), buf);
    GameLog(1, 2, (const char*)(lbl_803865F0 + 0x594), buf);
    if (DVDOpen(buf, lbl_8051DAD0 + 0xC0) == 0) return -1;
    lbl_805FBB9C = 0x10020;
    lbl_805FBB98 = (u8*)MEMAllocFromAllocator(&lbl_803F2870, 0x10020u);
    if (lbl_805FBB98 == NULL) return -1;
    memset(lbl_805FBB98, 0, lbl_805FBB9C);
    s32 size = *(s32*)(lbl_8051DAD0 + 0xF4);
    *(s16*)(lbl_8051DAD0 + 0x138) = 0;
    u8* aligned = (u8*)(((uintptr_t)lbl_805FBB98 + 0x1F) & ~(uintptr_t)0x1F);
    lbl_805FBB94 = aligned;
    *(u8**)(lbl_8051DAD0 + 0x154) = aligned;
    *(s32*)(lbl_8051DAD0 + 0x13C) = 0;
    *(s32*)(lbl_8051DAD0 + 0x140) = size;
    *(s32*)(lbl_8051DAD0 + 0x144) = 0;
    s32 chunk = 0x10000;
    if (size <= 0x10000) chunk = size;
    s32 head = chunk & ~0x1F;
    u32 rest = (u32)(0x10000 - head);
    u8* tail = *(u8**)(lbl_8051DAD0 + 0x154) + head;
    memset(tail, 0, rest);
    DCFlushRange(*(u8**)(lbl_8051DAD0 + 0x154) + head, rest);
    *(s32*)(lbl_8051DAD0 + 0x174) = 7;
    return 1;
}

// 0x80204838
// Draws the movie subtitle lines: picks the subtitle block for the current playback time and
// prints its lines bottom-up, centred horizontally above the static record's baseline.
void fn_80204838(u8* arg0) {
    s32 spC, sp8;
    fn_8015D660();
    fn_8024BDD4(0, NULL, 0, NULL);
    f32 ratio = (f32)*(u32*)(arg0 + 0x14) / (f32)*(u32*)(arg0 + 0x18);
    s32 block = fn_801C6564((f32)*(s32*)(arg0 + 0x10) / ratio);
    if (block < 0) return;
    s32 count = fn_801C6604(block);
    for (s32 i = count - 1; i >= 0; i--) {
        const char* text = fn_801C6618(block, i);
        fn_80188EDC(text, &spC, &sp8);
        s32 w = *(s32*)lbl_80412BE0 - spC;
        lbl_805FB62C = *(s32*)(lbl_80412BE0 + 4) - ((count - i) + 2) * lbl_805FB63C;
        lbl_805FB628 = (w + (s32)((u32)w >> 31)) >> 1;
        fn_80188F00(lbl_805F98F8, text);
    }
    fn_80188E5C((u8*)fn_802044C0);
}

}  // extern "C"
