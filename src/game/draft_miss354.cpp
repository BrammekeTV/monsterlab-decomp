// Drafts miss354: network/movie setup (0x801F2EE4), audio stream refill and voice start
// (0x80201BD4, 0x80203ABC), a textured quad draw (0x802044C0), audio-mixer buffer setup
// (0x80205454), listener pan/volume helpers (0x802095B0, 0x8020B1E0) and the battle AI action
// pick (0x8020E04C).
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "revolution.h"
#include "game/part.h"

typedef void (*VFn)(void*);

// Pair returned in r3/r4 (object pointer, action id); same layout as in draft_miss324.cpp.
struct ChoicePair {
    u8* p;
    u32 v;
};

extern "C" {
s32   DVDReadAsync(u8* fileInfo, void* addr, s32 len, s32 offset, void* cb, s32 prio);
void* MemAllocSmall(u32 size);                                  // 0x801D41CC
s32   fn_800DB944(f32* out, u8* obj);                           // 0x800DB944
s32   fn_80146930(u8* file, void* buf, s32 size);               // 0x80146930
void* fn_801B7198(void* anim, s32 a);                           // 0x801B7198
void* fn_801D40E0(s32 n);                                       // 0x801D40E0
void  fn_801F5E3C(void* a, const void* b, s32 c);               // 0x801F5E3C
void  fn_801F5E44(void);                                        // 0x801F5E44
void  fn_801F5E48(void* a, s32 b);                              // 0x801F5E48
void  fn_801F5E4C(void* a, s32 b);                              // 0x801F5E4C
void  fn_80200494(u8* obj, f32 a, f32 b, f32 c, f32 d);         // 0x80200494 (args in f1..f4, none in r4)
void  fn_80201A94(s32 result, u8* fileInfo);                                     // 0x80201A94 (DVD read callback)
void  fn_80201AD8(u8* audio, u8* state);                        // 0x80201AD8
void  fn_80203DE8(u8* audio, u8* state);                        // 0x80203DE8
s32   fn_80205208(u8* buf, u8* texObj, s32* outA, s32* outB, s32 n);  // 0x80205208 (returns the bytes used)
void  fn_80205310(u8* p, s32* a, s32* b, s32* c, s32* d, s32* e, u32* f, s32* g);  // 0x80205310
void  fn_80208D00(u8* obj, void* a, void* b);                   // 0x80208D00
void* fn_802080B8(void* self);                                  // 0x802080B8 (constructor, returns this)
s32   fn_8020DD90(u8* obj, s32 a);                              // 0x8020DD90
void  fn_80248050(f32* mtx);                                    // 0x80248050
void  fn_80248420(f32* mtx, f32 x, f32 y, f32 z);               // 0x80248420
void  C_MTXOrtho(f32* mtx, f32 a, f32 b, f32 c, f32 d, f32 e, f32 f);  // 0x8024849C
void  GXSetVtxDesc(s32 a, s32 b);                                // 0x8024A3A4
void  GXClearVtxDesc(void);                                        // 0x8024A7E4
void  GXSetVtxAttrFmt(s32 a, s32 b, s32 c, s32 d, s32 e);           // 0x8024A818
void  GXSetTexCoordGen2(s32 coord, s32 func, s32 src, s32 mtx, s32 normalize, s32 postMtx);  // 0x8024AC4C
void  GXSetNumTexGens(s32 a);                                       // 0x8024AE74
void  GXBegin(s32 a, s32 b, s32 c);                         // 0x8024B974
void  GXSetCullMode(s32 a);                                       // 0x8024BB64
void  GXSetChanAmbColor(s32 chan, u8* color);                         // 0x8024C140
void  GXSetNumChans(s32 a);                                       // 0x8024C2F0
void  GXSetChanCtrl(s32 chan, s32 enable, s32 ambSrc, s32 matSrc, s32 lightMask, s32 diffFn, s32 attnFn);  // 0x8024C314
void  GXInitTexObj(u8* obj, void* image, s32 w, s32 h, s32 fmt, s32 wrapS, s32 wrapT, s32 mip);  // 0x8024C3B8
void  GXInitTexObjCI(u8* obj, void* image, s32 w, s32 h, s32 fmt, s32 wrapS, s32 wrapT, s32 mip, s32 extra);  // 0x8024C5C4
void  GXInitTexObjLOD(u8* obj, s32 a, s32 b, s32 c, s32 d, s32 e, f32 f, f32 g, f32 h);  // 0x8024C60C
void  GXInitTexObjFilter(u8* obj, s32 a, s32 b);                       // 0x8024C72C
void  GXLoadTexObj(u8* obj, s32 id);                             // 0x8024C91C
void  fn_8024C970(u8* obj, void* lut, s32 fmt, s32 n);          // 0x8024C970
void  fn_8024C998(u8* obj, s32 id);                             // 0x8024C998
void  fn_8024D508(s32 a, s32 b);                                // 0x8024D508
void  fn_8024DA08(s32 a, s32 b, s32 c, s32 d);                  // 0x8024DA08
void  fn_8024DB64(s32 a);                                       // 0x8024DB64
void  GXSetBlendMode(s32 a, s32 b, s32 c, s32 d);                  // 0x8024DED8
void  GXSetZMode(s32 a, s32 b, s32 c);                         // 0x8024DF80
void  GXSetProjection(f32* mtx, s32 a);                             // 0x8024E1D0
void  GXLoadPosMtxImm(f32* mtx, s32 a);                             // 0x8024E280
void  GXSetCurrentMtx(s32 a);                                       // 0x8024E328
void  fn_8024E348(f32* mtx, s32 id, s32 type);                  // 0x8024E348
s32   fn_8025AC30(u8* voice, s32 a);                            // 0x8025AC30
void  AXSetVoiceState(u8* voice, s32 a);                            // 0x8025ACF0
void  fn_8025AD60(u8* voice, s32 a);                            // 0x8025AD60 (two arguments; r5 is not read)
s32   AXSetVoiceAddr(u8* voice, u16* p);                           // 0x8025B0A4
s32   fn_8025B19C(u8* voice, u16* p);                           // 0x8025B19C
s32   fn_8025B234(u8* voice, u16* p);                           // 0x8025B234
s32   fn_8025B330(u8* voice, u16* p);                           // 0x8025B330
void  fn_80260AD4(u8* voice, s32 a, s32 b, s32 c, s32 d, s32 e, s32 f, s32 g, s32 h);  // 0x80260AD4 (9th argument on the stack)
void* fn_802BE160(s32 size, s32 align);                         // 0x802BE160
void  fn_802BE220(void* h);                                     // 0x802BE220

extern const char lbl_805F9828[];   // 0x805F9828 ("Receive")
extern const char lbl_80385FC0[];   // 0x80385FC0 ("CreatedGameTrackerSema")
extern const char lbl_80385FD8[];   // 0x80385FD8 ("NetErrorManager")
extern u8   gAudio[];
extern u8   lbl_804DD140[];         // 0x804DD140 (network manager block)
extern u8   lbl_804EC590[];         // 0x804EC590 (movie packet ring)
extern u8   lbl_8051DAD0[];         // 0x8051DAD0 (audio stream state)
extern s32  lbl_805F97F8;           // 0x805F97F8
extern s32  lbl_805F97FC;           // 0x805F97FC
extern u8   lbl_805F98C1;           // 0x805F98C1
extern f32  lbl_805F9968;           // 0x805F9968 (camera angle override, 90.0 = unset)
extern f32  lbl_805FB91C;           // 0x805FB91C
extern s32  lbl_805FB950;           // 0x805FB950
extern s32  lbl_805FB954;           // 0x805FB954
extern s32  lbl_805FB958;           // 0x805FB958
extern u8*  lbl_805FB95C;           // 0x805FB95C
extern void* lbl_805FB964;          // 0x805FB964 (movie player)
extern void* lbl_805FB968;          // 0x805FB968
extern u32  lbl_805FB96C;           // 0x805FB96C
extern s32  lbl_805FB970;           // 0x805FB970
extern s32  lbl_805FB980;           // 0x805FB980
extern s32  lbl_805FB984;           // 0x805FB984
extern s32  lbl_805FB988;           // 0x805FB988
extern s32  lbl_805FB98C;           // 0x805FB98C
extern s32  lbl_805FB990;           // 0x805FB990
extern s32  lbl_805FB994;           // 0x805FB994
extern s32  lbl_805FB9A8;           // 0x805FB9A8
extern s32  lbl_805FB9AC;           // 0x805FB9AC
extern u32  lbl_805FB9B0;           // 0x805FB9B0
extern u32  lbl_805FB9B4;           // 0x805FB9B4
extern s32  lbl_805FB9B8;           // 0x805FB9B8
extern u32  lbl_805FB9BC;           // 0x805FB9BC
extern u32  lbl_805FB9C0;           // 0x805FB9C0
extern u32  lbl_805FB9C4;           // 0x805FB9C4
extern u32  lbl_805FB9C8;           // 0x805FB9C8
extern u32  lbl_805FB9CC;           // 0x805FB9CC
extern s32  lbl_805FB9D4;           // 0x805FB9D4
extern s32  lbl_805FB9D8;           // 0x805FB9D8
extern u32  lbl_805FB9DC;           // 0x805FB9DC
extern u8   lbl_805FB9E0[];         // 0x805FB9E0 (byte flag)
extern u32  lbl_805FB9E4;           // 0x805FB9E4
extern s32  lbl_805FB9E8;           // 0x805FB9E8
extern s32  lbl_805FB9F4;           // 0x805FB9F4
extern u8*  lbl_805FB9F8;           // 0x805FB9F8
extern u8   lbl_805FBA9C;           // 0x805FBA9C
extern s32  lbl_805FBAA8;           // 0x805FBAA8
extern s32  lbl_805FBAE0;           // 0x805FBAE0
extern u8   lbl_80600418;           // 0x80600418 (quad colour, red)
extern u8   lbl_80600419;           // 0x80600419 (quad colour, green)
extern u8   lbl_8060041A;           // 0x8060041A (quad colour, blue)
extern u8   lbl_8060041B;           // 0x8060041B (quad colour, alpha)

// Console physical address of a cached pointer (the DSP and the stream code work in these).
static inline u32 PhysAddr(const void* p) {
    intptr_t addr = (intptr_t)p;
    return (u32)addr + 0x80000000u;
}

// 0x801F2EE4
// Network manager reset: allocates the receive buffer and the movie player once, sets up the
// semaphores and clears every queue counter.
void fn_801F2EE4(void) {
    u8* net = lbl_804DD140;
    u8* sema = net + 0x40;
    u8* base = net + 0x20000;

    if (lbl_805FB9F8 == NULL) {
        lbl_805FB9F4 = 0x20000;
        lbl_805FB9F8 = (u8*)MemAllocSmall(0x20000U);
    }
    lbl_805FB9E0[0] = 0;
    if (lbl_805FB964 == NULL) {
        void* player = fn_801D40E0(0xC4);
        if (player != NULL) {
            player = fn_802080B8(player);
        }
        lbl_805FB964 = player;
        (*(VFn*)(*(u8**)player + 0xC))(player);
    }
    fn_801F5E3C(base - 0x1B60, lbl_805F9828, 1);
    lbl_805FB9D8 = 0;
    fn_801F5E44();
    fn_801F5E3C(net + 0x28, lbl_80385FD8, 1);
    lbl_805FB950 = 0;
    lbl_805FB954 = 0;
    lbl_805FB958 = 0;
    lbl_805FB980 = 0;
    lbl_805FB984 = 0;
    lbl_805FB988 = 0;
    lbl_805FB98C = 0;
    lbl_805FB970 = 1;
    lbl_805FB990 = 0;
    lbl_805FB95C = NULL;
    lbl_805F97F8 = 1;
    lbl_805F97FC = 1;
    fn_801F5E3C(sema + 0x3C4, lbl_80385FC0, 1);
    *(s32*)(sema + 0x3C0) = 0;
    fn_801F5E4C(sema + 0x3C4, 0);
    // Two groups of three slots (stride 0xA0 inside a 0x1E0 block): clear the in-use bytes and
    // keep the live-slot counter at +0x3C0 from going negative.
    u8* blk = sema;
    for (s32 i = 0; i < 2; i++) {
        if (*(u8*)(blk + 0x94) != 0) {
            *(u8*)(blk + 0x94) = 0;
            s32 n = *(s32*)(sema + 0x3C0) - 1;
            *(s32*)(sema + 0x3C0) = n;
            if (n < 0) *(s32*)(sema + 0x3C0) = 0;
        }
        if (*(u8*)(blk + 0x134) != 0) {
            *(u8*)(blk + 0x134) = 0;
            s32 n = *(s32*)(sema + 0x3C0) - 1;
            *(s32*)(sema + 0x3C0) = n;
            if (n < 0) *(s32*)(sema + 0x3C0) = 0;
        }
        if (*(u8*)(blk + 0x1D4) != 0) {
            *(u8*)(blk + 0x1D4) = 0;
            s32 n = *(s32*)(sema + 0x3C0) - 1;
            *(s32*)(sema + 0x3C0) = n;
            if (n < 0) *(s32*)(sema + 0x3C0) = 0;
        }
        blk += 0x1E0;
    }
    fn_801F5E48(net + 0x40 + 0x3C4, 0);
    u8* peer = net + 0x428;
    *(s32*)(peer + 0xC) = -1;
    *(s32*)(peer + 0x8) = -1;
    *(s32*)(lbl_804EC590 + 0xC) = -1;
    *(s32*)(lbl_804EC590 + 0x8) = -1;
    lbl_805FB9A8 = 0;
    if (lbl_805FB970 != 0) {
        if (lbl_805FB9D8 == 0) {
            fn_801F5E4C(base - 0x1B60, 6);
        }
        lbl_805FB968 = NULL;
        if (lbl_805FB9D8 == 0) {
            fn_801F5E48(base - 0x1B60, 6);
        }
    }
    lbl_805FB9AC = 0;
    lbl_805FB96C = 0;
    lbl_805FB9D4 = 0;
    lbl_805FB9B0 = 0;
    lbl_805FB9B4 = 0;
    lbl_805FB9B8 = 0;
    lbl_805FB9BC = 0;
    lbl_805FB9C0 = 0;
    lbl_805FB9C4 = 0;
    lbl_805FB9C8 = 0;
    lbl_805FB9CC = 0;
    u8* ringB = base - 0x1B7C;
    u8* ringA = base - 0x1B98;
    u8* misc = base - 0x1B48;
    *(s32*)ringB = 0;
    *(s32*)(ringB + 0xC) = 0;
    *(s32*)(ringB + 0x8) = 0;
    *(s32*)(ringB + 0x10) = 0;
    *(s32*)ringA = 0;
    *(s32*)(ringA + 0xC) = 0;
    *(s32*)(ringA + 0x8) = 0;
    *(s32*)(ringA + 0x10) = 0;
    lbl_805FB994 = 0;
    lbl_805FB9DC = 0;
    lbl_805FB9E4 = 0;
    lbl_805FB9E8 = (s32)lbl_805FB91C;
    *(u8*)misc = 0;
    *(u8*)(misc + 0x80) = 0;
    *(s32*)(misc + 0x98) = 0;
}

// 0x80201BD4
// Refills the second half of the audio stream buffer: zero-pads past the data that was read,
// starts the DVD read (or the synchronous read when no disc is attached), updates the
// remaining/offset counters and, at the very start, primes the loop info for the voice.
void fn_80201BD4(u8* arg0, u8* arg1) {
    u16 loop[3];
    s32 saved = lbl_805FBAE0;
    lbl_805FBAE0 = 1;
    if (*(u8**)(arg1 + 0x94) == NULL ||
        (*(u8**)(arg1 + 0x98) == NULL && *(s32*)(arg1 + 0xA0) == 2)) {
        *(s32*)(arg1 + 0xB8) = 0;
        lbl_805FBAE0 = saved;
        return;
    }
    if (*(s32*)(arg1 + 0xA0) == 2) {
        s32 remain = *(s32*)(arg1 + 0x80);
        u8* dst = *(u8**)(arg1 + 0x98) + *(s32*)(arg1 + 0xA4);
        s32 n = 0x8000;
        if (remain <= 0x8000) n = remain;
        s32 len = n & ~0x1F;
        u32 pad = 0x8000 - len;
        u8* padPtr = dst + len;
        memset(padPtr, 0, pad);
        DCFlushRange(padPtr, pad);
        if (lbl_805F98C1 != 0) {
            DVDReadAsync(arg1 + 0x3C, dst, len, *(s32*)(arg1 + 0x84), (void*)fn_80201A94, 2);
        } else {
            fn_80146930(arg0 + 0x1E4, dst, len);
            DCFlushRange(dst, (u32)len);
            // Both tests are address-of-member checks in the original and fold to constants.
            if ((lbl_8051DAD0 + 0x3C) == NULL) {
                fn_80201AD8(gAudio, lbl_8051DAD0);
            } else if ((lbl_8051DAD0 + 0xFC) == NULL) {
                fn_80201AD8(gAudio, lbl_8051DAD0 + 0xC0);
            }
        }
        *(s32*)(arg1 + 0x80) -= len;
        *(s32*)(arg1 + 0x84) += len;
    }
    if (*(s32*)(arg1 + 0xA4) == 0 && *(u8**)(arg1 + 0x8C) != NULL) {
        loop[0] = **(u8**)(arg1 + 0x94);
        loop[1] = 0;
        loop[2] = 0;
        fn_8025B330(*(u8**)(arg1 + 0x8C), loop);
    }
    if (*(s32*)(arg1 + 0xA0) == 1) {
        fn_80201AD8(gAudio, arg1);
    }
    lbl_805FBAE0 = saved;
}

// 0x80203ABC
// Starts the stream voices: sets up the buffer addresses, programs each voice (address, loop
// and coefficient blocks, sample-rate ratio), then marks the stream as running.
void fn_80203ABC(u8* arg0, u8* arg1) {
    u16 src[7];
    u16 addr[8];
    u16 coef[20];

    if (arg1 == NULL) return;
    if (*(s32*)(arg1 + 0xB4) != 0) return;
    s32 remain = *(s32*)(arg1 + 0x80);
    s32 n = 0x10000;
    *(s32*)(arg1 + 0xB4) = 1;
    if (remain <= 0x10000) n = remain;
    if ((remain & ~0x1F) == 0) {
        *(u32*)(arg1 + 0x88) = PhysAddr(*(u8**)(arg1 + 0x94)) + (u32)(n & ~0x1F);
        *(s32*)(arg1 + 0x7C) += 1;
    }
    u32 half = PhysAddr(*(u8**)(arg1 + 0x94)) + 0x8000;
    *(u32*)(arg1 + 0xA8) = half;
    *(u32*)(arg1 + 0xAC) = half;
    if (*(u8**)(arg1 + 0x8C) == NULL ||
        (*(u8**)(arg1 + 0x90) == NULL && *(s32*)(arg1 + 0xA0) == 2)) {
        fn_80203DE8(arg0, arg1);
        return;
    }
    u8* slot = arg1;
    for (s32 i = 0; i < *(s32*)(arg1 + 0xA0); i++) {
        u8* hdr = *(u8**)(slot + 0x94);
        if (hdr != NULL) {
            u8* voice = *(u8**)(slot + 0x8C);
            u32 base = PhysAddr(hdr) << 1;
            u32 endAddr = base + 0x1FFFF;
            u32 curAddr = base + *(u32*)(hdr + 0x18) + 0xC0;
            u32 startAddr = base + 2;
            fn_80260AD4(voice, 0, 0, -0x388, -0x388, -0x388, 0, 0x7F, 0);
            addr[0] = 1;
            addr[1] = 0;
            addr[2] = (u16)(startAddr >> 16);
            addr[3] = (u16)startAddr;
            addr[4] = (u16)(endAddr >> 16);
            addr[5] = (u16)endAddr;
            addr[6] = (u16)(curAddr >> 16);
            addr[7] = (u16)curAddr;
            for (s32 k = 0; k < 20; k++) {
                coef[k] = *(u16*)(hdr + 0x1C + k * 2);
            }
            u32 ratio = (u32)(65536.0f * ((f32) * (u32*)(hdr + 0x8) / 32000.0f));
            src[0] = (u16)(ratio >> 16);
            src[1] = (u16)ratio;
            src[2] = 0;
            src[3] = 0;
            src[4] = 0;
            src[5] = 0;
            src[6] = 0;
            fn_8025AD60(voice, 1);
            fn_8025B19C(voice, coef);
            AXSetVoiceAddr(voice, addr);
            fn_8025AC30(voice, 1);
            fn_8025B234(voice, src);
            if (*(u32*)(arg1 + 0x9C) == 1U) {
                AXSetVoiceState(voice, 1);
            }
        }
        slot += 4;
    }
    u32 mode = *(u32*)(arg1 + 0x9C);
    *(s32*)(arg1 + 0xB4) = 2;
    if (mode == 0) {
        *(u8*)(arg0 + 0x29) = 1;
        *(u8*)(arg0 + 0x1D1) = 1;
    } else if (mode == 1) {
        lbl_805FBAA8 = 1;
    }
}

// 0x802044C0
// Draws a textured, vertex-coloured quad in screen space: x0/x1/y0/y1 are the corners,
// u0/u1/v0/v1 the texture coordinates, color is packed with the red byte lowest.
void fn_802044C0(u8* tex, u32 color, f32 x0, f32 x1, f32 y0, f32 y1, f32 u0, f32 u1, f32 v0, f32 v1) {
    f32 view[12];
    f32 proj[16];
    f32 scale[12];
    u8 tlut[0x10];
    u8 texObj[0x20];
    u8 ambient[4];

    GXClearVtxDesc();
    GXSetVtxAttrFmt(4, 9, 0, 4, 0);
    GXSetVtxAttrFmt(4, 0xB, 0, 1, 0);
    GXSetVtxAttrFmt(4, 0xD, 1, 4, 0);
    GXSetVtxDesc(9, 1);
    GXSetVtxDesc(0xB, 1);
    GXSetVtxDesc(0xD, 1);
    ambient[0] = lbl_80600418;
    ambient[1] = lbl_80600419;
    ambient[2] = lbl_8060041A;
    ambient[3] = lbl_8060041B;
    GXSetChanAmbColor(0, ambient);
    GXSetChanCtrl(0, 1, 0, 1, 1, 2, 1);
    GXSetNumChans(1);
    GXSetNumTexGens(1);
    fn_8024DB64(1);
    fn_8024D508(0, 0);
    fn_8024DA08(0, 0, 0, 4);
    GXSetZMode(1, 3, 1);
    GXSetCullMode(0);
    GXSetCurrentMtx(3);
    fn_80248050(view);
    GXLoadPosMtxImm(view, 3);
    C_MTXOrtho(proj, 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 500.0f);
    GXSetProjection(proj, 1);
    fn_8024C970(tlut, *(void**)(tex + 0x1C), *(u8*)(tex + 0x11), 0x10);
    fn_8024C998(tlut, 0);
    GXInitTexObjCI(texObj, *(void**)(tex + 0x18), *(u16*)(tex + 0x0), *(u16*)(tex + 0x2), *(u8*)(tex + 0x10), 0, 0, 0, 0);
    GXLoadTexObj(texObj, 0);
    GXInitTexObjFilter(texObj, 0, 0);
    fn_80248420(scale, 1.0f, 1.0f, 1.0f);
    fn_8024E348(scale, 0x1E, 1);
    GXSetNumTexGens(1);
    GXSetTexCoordGen2(0, 1, 4, 0x1E, 0, 0x7D);
    GXSetBlendMode(1, 4, 5, 0);
    GXBegin(0x80, 4, 4);
    u8 c0 = (u8)color;
    u8 c1 = (u8)(color >> 8);
    u8 c2 = (u8)(color >> 16);
    GXWGFifo._f32 = x0;
    GXWGFifo._f32 = y0;
    GXWGFifo._u8 = c0;
    GXWGFifo._u8 = c1;
    GXWGFifo._u8 = c2;
    GXWGFifo._f32 = u0;
    GXWGFifo._f32 = v0;
    GXWGFifo._f32 = x1;
    GXWGFifo._f32 = y0;
    GXWGFifo._u8 = c0;
    GXWGFifo._u8 = c1;
    GXWGFifo._u8 = c2;
    GXWGFifo._f32 = u1;
    GXWGFifo._f32 = v0;
    GXWGFifo._f32 = x1;
    GXWGFifo._f32 = y1;
    GXWGFifo._u8 = c0;
    GXWGFifo._u8 = c1;
    GXWGFifo._u8 = c2;
    GXWGFifo._f32 = u1;
    GXWGFifo._f32 = v1;
    GXWGFifo._f32 = x0;
    GXWGFifo._f32 = y1;
    GXWGFifo._u8 = c0;
    GXWGFifo._u8 = c1;
    GXWGFifo._u8 = c2;
    GXWGFifo._f32 = u0;
    GXWGFifo._f32 = v1;
}

// 0x80205454
// Allocates one 128-byte-aligned block for the four texture planes of every mixer slot, inits
// the texture objects over it, then lays out the two trailing lookup textures.
// Returns 1 on success, 0 if the size is out of range or the allocation failed.
s32 fn_80205454(u8* arg0) {
    s32 planeB;
    s32 planeA;
    s32 planeC;
    s32 stride;
    s32 extra;
    u32 width;
    s32 height;

    fn_80205310(arg0 + 0x100, &planeB, &planeA, &planeC, &stride, &extra, &width, &height);
    if (width > 0x400U) {
        return 0;
    }
    u8* mem = (u8*)fn_802BE160(extra + stride * *(s32*)(arg0 + 0x100) + 0x7F, stride);
    if (mem != NULL) {
        *(u8**)(arg0 + 0x178) = mem;
        u8* rec = arg0;
        u8* obj = arg0;
        u8* cur = (u8*)(((uintptr_t)mem + 0x7F) & ~(uintptr_t)0x7F);
        for (s32 i = 0; i < *(s32*)(arg0 + 0x100); i++) {
            if (*(s32*)(rec + 0x118) != 0) {
                *(u8**)(rec + 0x11C) = cur;
                *(u32*)(rec + 0x120) = width;
                GXInitTexObj(obj, cur, (u16)width, (u16)*(u32*)(arg0 + 0x108), 1, 0, 0, 0);
                GXInitTexObjLOD(obj, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f);
            }
            if (*(s32*)(rec + 0x124) != 0) {
                *(u8**)(rec + 0x128) = cur + planeB;
                *(s32*)(rec + 0x12C) = height;
                GXInitTexObj(obj + 0x20, cur + planeB, (u16)height, (u16)*(u32*)(arg0 + 0x110), 1, 0, 0, 0);
                GXInitTexObjLOD(obj + 0x20, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f);
            }
            if (*(s32*)(rec + 0x130) != 0) {
                *(u8**)(rec + 0x134) = cur + planeA;
                *(s32*)(rec + 0x138) = height;
                GXInitTexObj(obj + 0x40, cur + planeA, (u16)height, (u16)*(u32*)(arg0 + 0x110), 1, 0, 0, 0);
                GXInitTexObjLOD(obj + 0x40, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f);
            }
            if (*(s32*)(rec + 0x13C) != 0) {
                *(u8**)(rec + 0x140) = cur + planeC;
                *(u32*)(rec + 0x144) = width;
                GXInitTexObj(obj + 0x60, cur + planeC, (u16)width, (u16)*(u32*)(arg0 + 0x108), 1, 0, 0, 0);
                GXInitTexObjLOD(obj + 0x60, 0, 0, 0, 0, 0, 0.0f, 0.0f, 0.0f);
            }
            rec += 0x30;
            obj += 0x80;
            cur += stride;
        }
        *(s32*)(arg0 + 0x17C) = stride;
        // The first lookup texture is placed after the planes, the second after the first.
        s32 used = fn_80205208(cur, arg0 + 0x190, (s32*)(arg0 + 0x180), (s32*)(arg0 + 0x184), *(s32*)(arg0 + 0x104));
        fn_80205208(cur + used, arg0 + 0x1B0, (s32*)(arg0 + 0x188), (s32*)(arg0 + 0x18C), *(s32*)(arg0 + 0x10C));
        return 1;
    }
    u8* old = *(u8**)(arg0 + 0x178);
    if (old != NULL) {
        fn_802BE220(old);
        *(u8**)(arg0 + 0x178) = NULL;
    }
    return 0;
}

// 0x802095B0
// Computes the camera-relative orientation for a listener slot of type 1/4..7 (sets the two
// output scales and calls fn_80200494 with the angle) or fetches a position for type 3.
// Returns 1 if an argument or lookup is missing, else 0.
s32 fn_802095B0(u8* arg0, void* arg1, void* arg2, void* arg3) {
    f32* outA = (f32*)arg1;
    f32* outB = (f32*)arg2;
    s32 sp8;

    if (arg0 == NULL || arg1 == NULL || arg2 == NULL || arg3 == NULL) {
        return 1;
    }
    s32 type = *(s32*)(arg0 + 0x10);
    if (type == 1 || (u32)(type - 4) <= 3U) {
        f32 angle = 0.0f;
        if (lbl_805F9968 != 90.0f) {
            angle = lbl_805F9968;
        } else {
            switch (lbl_805FBA9C) {
            case 1:
                angle = 45.0f;
                break;
            case 2:
                angle = 60.0f;
                break;
            }
        }
        switch (type) {
        case 1:
            *outA = 0.5f;
            *outB = 0.5f;
            break;
        case 4:
        case 6:
            fn_80200494(arg0, 0.017453292f * -angle, 0.0f, 1.0f, 0.0f);
            *outA = 1.0f;
            *outB = 0.0f;
            break;
        case 5:
        case 7:
            if (lbl_805FBA9C == 0) {
                fn_80200494(arg0, 0.017453292f * angle, 0.0f, 10000.0f, 0.0f);
            } else {
                fn_80200494(arg0, 0.017453292f * angle, 0.0f, 1.0f, 0.0f);
            }
            *outA = 0.0f;
            *outB = 1.0f;
            break;
        }
    } else {
        if (type == 3) {
            sp8 = *(s32*)(arg0 + 0x44);
            u8* anim = (u8*)fn_801B7198(&sp8, 0);
            if (anim == NULL) return 1;
            f32* pos = *(f32**)(anim + 0x4);
            if (pos == NULL) return 1;
            *(f32*)(arg0 + 0x18) = pos[0];
            *(f32*)(arg0 + 0x1C) = pos[1];
            *(f32*)(arg0 + 0x20) = pos[2];
        }
        if ((u32)(*(s32*)(arg0 + 0x10) - 2) <= 1U) {
            fn_80208D00(arg0, arg1, arg2);
        }
    }
    return 0;
}

// 0x8020B1E0
// Sets the sound source's volume (+0x18) from its distance to the listener: full inside the
// near radius (+0x15 of the config), fading to zero at the far radius (+0x16).
void fn_8020B1E0(u8* arg0, u8* arg1) {
    f32 pos[3];
    f32 lst[3];
    f32 tgt[3];
    f32 d[3];
    f32 len;
    s32 anim;

    pos[0] = 0.0f;
    pos[1] = 0.0f;
    pos[2] = 0.0f;
    if (arg1 != NULL) {
        pos[0] = *(f32*)(arg1 + 0x0);
        pos[1] = *(f32*)(arg1 + 0x4);
        pos[2] = *(f32*)(arg1 + 0x8);
    }
    if (fn_800DB944(lst, arg1) == 0) {
        *(f32*)(arg0 + 0x18) = 0.0f;
        return;
    }
    if (arg1 == NULL) {
        pos[0] = lst[0];
        pos[1] = lst[1];
        pos[2] = lst[2];
    }
    lst[0] = pos[0];
    lst[1] = pos[1];
    lst[2] = pos[2];
    u32 flags = *(u32*)(arg0 + 0x24);
    if (flags & 2) {
        anim = *(s32*)(arg0 + 0x4);
        u8* a = (u8*)fn_801B7198(&anim, 0);
        if (a != NULL) {
            f32* p = *(f32**)(a + 0x4);
            tgt[0] = p[0];
            tgt[1] = p[1];
            tgt[2] = p[2];
        } else {
            tgt[0] = lst[0];
            tgt[1] = lst[1];
            tgt[2] = lst[2];
        }
    } else if (flags & 1) {
        tgt[0] = *(f32*)(arg0 + 0x8);
        tgt[1] = *(f32*)(arg0 + 0xC);
        tgt[2] = *(f32*)(arg0 + 0x10);
    } else {
        *(f32*)(arg0 + 0x18) = 500.0f * *(f32*)(*(u8**)arg0 + 0x8);
        return;
    }
    d[0] = tgt[0] - lst[0];
    d[1] = tgt[1] - lst[1];
    d[2] = tgt[2] - lst[2];
    f32 lenSq = d[2] * d[2] + (d[0] * d[0] + d[1] * d[1]);
    len = 0.0f;
    if (lenSq > 0.0f) {
        len = sqrtf(lenSq);  // original: frsqrte with two Newton steps
    }
    u8* cfg = *(u8**)arg0;
    u8 nearR = *(u8*)(cfg + 0x15);
    u8 farR = *(u8*)(cfg + 0x16);
    f32 nearF = (f32)nearR;
    f32 lenSqR = len * len;
    f32 lo;
    f32 hi;
    if (lenSqR < nearF * nearF) {
        lo = ((nearF - len) / nearF) * 0.5f;
        hi = 1.0f - lo;
    } else if (lenSqR < (f32)farR * (f32)farR) {
        lo = 0.0f;
        hi = 1.0f - ((len - nearF) / (f32)((s32)farR - (s32)nearR));
    } else {
        *(f32*)(arg0 + 0x18) = 0.0f;
        return;
    }
    *(f32*)(arg0 + 0x18) = *(f32*)(cfg + 0x8) * (lo + (hi - lo));
}

// 0x8020E04C
// Picks one of the object's candidate actions (count at +0x84, pairs at +0x34) according to
// the mode from fn_8020DD90: mode 0 the highest signed half-word at +0xE of the action record,
// mode 1 the highest signed byte at +0x10, mode 2 resets all scores to 1.0 and takes the pick
// of fn_8020DD90; anything else returns entry 0. Also loads three signed bytes of the
// object's action-data record (table at data + 0x1D20) into float slots +0xC/+0x10/+0x14.
ChoicePair fn_8020E04C(u8* arg0, s32 arg1) {
    s32 i;
    s32 best;
    s32 bestIdx;
    ChoicePair r;

    *(f32*)(arg0 + 0xC) = (f32)*(s8*)(*(u8**)(gGameDb->data + 0x1D20) + arg1 * 8 + 4);
    *(f32*)(arg0 + 0x10) = (f32)*(s8*)(*(u8**)(gGameDb->data + 0x1D20) + arg1 * 8 + 5);
    *(f32*)(arg0 + 0x14) = (f32)*(s8*)(*(u8**)(gGameDb->data + 0x1D20) + arg1 * 8 + 6);
    s32 mode = fn_8020DD90(arg0, 3);
    switch (mode) {
    case 0: {
        s32 count = *(s32*)(arg0 + 0x84);
        best = -1;
        bestIdx = -1;
        for (i = 0; i < count; i++) {
            u8* act = *(u8**)(gGameDb->data + 0x430) + *(s32*)(arg0 + 0x38 + i * 8) * 0x14;
            s32 v = *(s16*)(act + 0xE);
            if (best < v) {
                best = v;
                bestIdx = i;
            }
        }
        break;
    }
    case 1: {
        s32 count = *(s32*)(arg0 + 0x84);
        best = -1;
        bestIdx = -1;
        for (i = 0; i < count; i++) {
            u8* act = *(u8**)(gGameDb->data + 0x430) + *(s32*)(arg0 + 0x38 + i * 8) * 0x14;
            s32 v = *(s8*)(act + 0x10);
            if (best < v) {
                best = v;
                bestIdx = i;
            }
        }
        break;
    }
    case 2:
        for (i = 0; i < *(s32*)(arg0 + 0x84); i++) {
            *(f32*)(arg0 + 0xC + i * 4) = 1.0f;
        }
        bestIdx = fn_8020DD90(arg0, *(s32*)(arg0 + 0x84));
        break;
    default:
        r.p = *(u8**)(arg0 + 0x34);
        r.v = *(u32*)(arg0 + 0x38);
        return r;
    }
    r.p = *(u8**)(arg0 + 0x34 + bestIdx * 8);
    r.v = *(u32*)(arg0 + 0x38 + bestIdx * 8);
    return r;
}

}  // extern "C"
