// Mixed helpers (0x8007xxxx - 0x8016xxxx): level object-table scans, MIDI/wavetable file loading,
// GX capture-layer rendering, and a few small record setups.
#include "types.h"
#include "revolution.h"
#include "engine/math.h"

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

typedef void (*VCallFn)(void* obj, s32 arg);  // vtable +0x18: per-object init call
typedef s32 (*VQueryFn)(void* obj);           // vtable +0xBC

extern "C" {
void* MemAllocSmall(u32 size);                                          // 0x801D41CC
void QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);         // 0x80145EE8
u32 RandomU32(void);
void ScopedLockCtor(ScopedLock* l);                                     // 0x801F5384
void ScopedLockDtor(ScopedLock* l, s32 flags);                          // 0x801F53B0
u32 fn_80044C0C(void);
void fn_8008B850(u8* p);                                                // 0x8008B850
void fn_8008C8C8(u8* a, u8* vec);                                       // 0x8008C8C8
void fn_8008CAA4(u8* a);                                                // 0x8008CAA4
void fn_8008CCC4(u8* a);                                                // 0x8008CCC4
void fn_8008D244(u8* a, u8* b);                                         // 0x8008D244 (stores b at +0x284)
void fn_8008D24C(u8* a, u8* b);                                         // 0x8008D24C (stores b at +0x288)
void fn_8008D6BC(u8* a, u32 hash);                                      // 0x8008D6BC
void fn_800975D0(u8* dst, u8* nameBytes, u8* owner, s32 index);         // 0x800975D0
void fn_80097A18(u8* obj, u8* owner, s32 a, s32 b);                     // 0x80097A18
void fn_800980C8(u8* obj, s32 mode);                                    // 0x800980C8
void fn_800980F8(u8* p, s32 b);                                         // 0x800980F8
f32 fn_80098700(u8* p);                                                 // 0x80098700
void fn_80098E74(u8* p);                                                // 0x80098E74
void fn_80146808(u8* file, const char* name, const char* mode);         // 0x80146808
s32 fn_80146930(u8* file, void* buf, s32 size);                         // 0x80146930
void fn_801469FC(u8* file);                                             // 0x801469FC
s32 fn_80146A58(u8* file);                                              // 0x80146A58
void fn_8014950C(s32 a, s32 b);                                         // 0x8014950C
void fn_80149594(void);                                                 // 0x80149594
void fn_8015846C(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10,
                 u8 a11, u32 a12, u32 a13);                             // 0x8015846C
void fn_80158BC4(s32 extra);                                            // 0x80158BC4
void fn_80158C34(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10,
                 u32 a11, u32 a12);                                     // 0x80158C34
void fn_80158C94(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10,
                 u32 a11, u32 a12, u32 a13);                            // 0x80158C94
void fn_80158D58(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10,
                 u32 a11, u32 a12, u32 a13, u32 a14, u32 a15);          // 0x80158D58
u32 fn_80158E64(s32 a3, s32 a4);                                        // 0x80158E64
void fn_80158E88(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10,
                 u32 a11, u32 a12, u32 a13);                            // 0x80158E88
void fn_80158F04(u32 a3, s32 a4, s32* tbl, s32 a6);                     // 0x80158F04
void fn_80158FBC(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8);       // 0x80158FBC
void fn_80158FF4(u32 a3, u32 a4, u32 a5, u32 a6, u32 a7, u32 a8, u32 a9, u32 a10, u32 a11);  // 0x80158FF4
void fn_80158228(u8* p, s32 a, s32 b);                                  // 0x80158228
void fn_80158530(u8* a, u8* b, u8** outBuf, s32* outSize, f32 f1, f32 f2);  // 0x80158530
void fn_801584E8(u8* p, u32 idx);                                       // 0x801584E8
void fn_80158870(u8* p, s32 a4, s32 unused5, s32 unused6, s32 unused7, s32 a8, s32 a9, s32 a10);  // 0x80158870
u32 fn_80159058(void);                                                  // 0x80159058
u32 fn_80159084(s32 a3, u8* a4);                                        // 0x80159084
void fn_801590A0(void);                                                 // 0x801590A0
void fn_80152D84(u32 a, u32 b, s32 c, u32 d, u8* e, u32 f, u32 g, u32 h);  // 0x80152D84
void fn_80159258(void);                                                 // 0x80159258
void fn_801578D8(u8* p, u32 a, s32 mode);                               // 0x801578D8
void fn_80157B28(u8* p, u32 a, s32 mode);                               // 0x80157B28
void fn_8015EB3C(u8* p);                                                // 0x8015EB3C
void* memset(void* p, int c, unsigned long n);
char* strcat(char* dst, const char* src);
char* strcpy(char* d, const char* s);

extern u8 gWorldState[];                // 0x803E6D00
extern const char lbl_805F88F0[];       // ".mid"
extern const char lbl_805F88F8[];       // "rb"
extern const char lbl_805F88FC[];       // ".wt"
extern const char lbl_805F8900[];       // ".pcm"
extern s32 lbl_80348938[6];             // 0x80348938
extern u8 lbl_80348950[];               // 0x80348950 (0x20-byte records, u32 at +0)
extern u8 lbl_80348A10[];               // 0x80348A10 (0x20-byte records, s32 at +0)
extern u32 lbl_80348AD0;                // 0x80348AD0
extern u8 lbl_8035C1D0[];               // 0x8035C1D0 (27 byte pairs)
extern u8 lbl_80401C30[];               // 0x80401C30
extern u8 lbl_80401C40[];               // 0x80401C40
extern u8 lbl_80403740[];               // 0x80403740
extern u8 lbl_8040AC50[];               // 0x8040AC50
extern u8 lbl_8040ACB0[];               // 0x8040ACB0
extern s32 lbl_8040ADF0[8];             // 0x8040ADF0
extern s32 lbl_8040AE10[8];             // 0x8040AE10
extern s32 lbl_805FB4B0;                // 0x805FB4B0
extern s32 lbl_805FB518;                // 0x805FB518
extern void* lbl_805FB5DC;              // 0x805FB5DC
extern f32 lbl_805FEF68;                // 0x805FEF68 (1.0f)
extern f32 lbl_805FEF6C;                // 0x805FEF6C (0.0f)
extern f32 lbl_805FEF70;                // 0x805FEF70 (+0.0001f)
extern f32 lbl_805FEF74;                // 0x805FEF74 (-0.0001f)
extern f32 lbl_805FF1BC;                // 0x805FF1BC
extern f32 lbl_805FF228;                // 0x805FF228
extern f32 lbl_805FF22C;                // 0x805FF22C
extern f32 lbl_805FF250;                // 0x805FF250
extern f32 lbl_805FF5E8;                // 0x805FF5E8 (0.0f)
extern f32 lbl_805FF604;                // 0x805FF604 (rotation angle)
}
extern "C++" s32 gZoneResourceSize;     // 0x805FB94C

static const u32 kHashSpiritNote   = 0xA9C19A04u;  // "spirit_noteA"
static const u32 kHashSpiritNote2  = 0xD00FB13Du;  // "spirit_note2A"
static const u32 kHashSpiritNote3  = 0xC914807Cu;  // "spirit_note3A"
static const u32 kHashSpiritNote4  = 0x865516BBu;  // "spirit_note4A"
static const u32 kHashSpiritNote5  = 0x9F4E27FAu;  // "spirit_note5A"
static const u32 kHashKeysRow1     = 0xAF178E4Bu;  // "keys_row_01A"
static const u32 kHashKeysRow2     = 0x843ADD88u;  // "keys_row_02A"
static const u32 kHashKeysRow3     = 0x9D21ECC9u;  // "keys_row_03A"
static const u32 kHashKeysRow4     = 0xD2607A0Eu;  // "keys_row_04A"
static const u32 kHashScoring      = 0x554E7F03u;  // "scoringA"
static const u32 kHashScoring2     = 0xBCC28EFBu;  // "scoring_02A"
static const u32 kHashScoring3     = 0xA5D9BFBAu;  // "scoring_03A"
static const u32 kHashScoring4     = 0xEA98297Du;  // "scoring_04A"
static const u32 kHashOrganPipe    = 0x1B982E81u;  // "organ_pipeA"
static const u32 kHashOrganPipe2   = 0x92635AAFu;  // "organ_pipe_02A"
static const u32 kHashOrganPipe3   = 0x8B786BEEu;  // "organ_pipe_03A"
static const u32 kHashOrganPipe4   = 0xC439FD29u;  // "organ_pipe_04A"
static const u32 kHashSpiritGhost  = 0xE934FF98u;  // "spirit_ghostA"
static const u32 kHashSkullblast1  = 0xF8A1591Bu;  // "skullblast_1A"
static const u32 kHashSkullblast2  = 0xD38C0AD8u;  // "skullblast_2A"
static const u32 kHashSkullblast3  = 0xCA973B99u;  // "skullblast_3A"
static const u32 kHashSkullblast4  = 0x85D6AD5Eu;  // "skullblast_4A"

static const u32 kHashCastleBowl   = 0x7279AE1Eu;  // "castle_brazier_bowlA"
static const u32 kHashCastleWand   = 0xE188D328u;  // "castle_brazier_wandA"
static const u32 kHashCastleEarth  = 0xB57069FEu;  // "castle_earth_symbolA"
static const u32 kHashCastleAir    = 0xA65DCF50u;  // "castle_air_symbolA"
static const u32 kHashCastleWater  = 0x8F25BB8Au;  // "castle_water_ballA"
static const u32 kHashCastleFire   = 0xBAA6441Fu;  // "castle_fire_ballA"
static const u32 kHashCastleGood   = 0x67B779E5u;  // "castle_good_ballA"
static const u32 kHashCastleBad    = 0xF3B2D25Eu;  // "castle_bad_symbolA"

static const u32 kHashReactorHeart     = 0xC0DE24D7u;  // "reactor_heartA"
static const u32 kHashReactorHammer    = 0xE1BE67D3u;  // "reactor_hammerA"
static const u32 kHashReactorScrewdrv  = 0x95E820FDu;  // "reactor_screwdriverA"
static const u32 kHashReactorBolts     = 0x92532DB8u;  // "reactor_boltsA"
static const u32 kHashReactorScrews    = 0xE812600Fu;  // "reactor_screwsA"
static const u32 kHashReactorLightning = 0x6BE98B20u;  // "reactor_lightningA"
static const u32 kHashReactorLights    = 0x8033D138u;  // "reactor_lightsA"
static const u32 kHashReactorLightsAlt = 0x193A8082u;  // unresolved; handled like the lights hash

// One BP register write (opcode 0x61 then the 32-bit word) through the write-gather pipe.
static void GxBpWrite(u32 value) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = value;
}

// Shared body of fn_801598AC / fn_80159C60: for each of the three capture layers that is enabled,
// programs the GX copy/texture registers and draws its 17 tile blocks.
static void RenderCaptureLayers(void) {
    fn_80159258();
    u32 cfg = lbl_80348AD0;
    s32* enabled = (s32*)(lbl_80401C30 + 0x2410);
    u8* tblB = lbl_80401C30 + 0x7520;
    u8* tblA = lbl_80401C30 + 0x2420;
    u8* rec = lbl_80401C30 + 0x10000 - 0x6EC0;
    for (s32 i = 0; i < 3; i++) {
        if (*enabled != 0) {
            GxBpWrite(0xFE000FE0);
            GxBpWrite(0x410004A0);
            GxBpWrite(0xFE000001);
            GxBpWrite(0x41000001);
            GxBpWrite(0xFE000060);
            GxBpWrite(((u32)*(u8*)(rec + 0x12) << 5) | 0x80000000u);
            if ((u32)lbl_8040ADF0[0] != *(u32*)(rec + 0x18) || lbl_8040AE10[0] != 0) {
                u32 src = *(u32*)(rec + 0x18);
                u8 idx = *(u8*)(rec + 0x13);
                lbl_805FB4B0 = lbl_805FB4B0 + 1;
                lbl_8040ADF0[0] = (s32)src;
                s32 second = *(s32*)(lbl_80348A10 + idx * 0x20);
                lbl_8040AE10[0] = 0;
                GxBpWrite((((u32)*(u8*)(rec + 0xF) - 1) & 0xF) << 12 | 0x84000000u);
                GxBpWrite(((u32)*(u16*)(rec + 0x4) - 1) | (((u32)*(u16*)(rec + 0x6) - 1) << 10) |
                          ((u32)*(u8*)(rec + 0x10) << 20) | 0x88000000u);
                GxBpWrite((*(u32*)(lbl_80348950 + idx * 0x20) >> 5) | 0x8C0D0000u | 0x8000u);
                if (second >= 0) {
                    GxBpWrite(((u32)second >> 5) | 0x900D0000u | 0x8000u);
                } else {
                    GxBpWrite(0x90000000u);
                }
                GxBpWrite(((*(u32*)(rec + 0x18) >> 5) & 0xFFFFFF) | 0x94000000u);
                if (*(u8*)(rec + 0xD) != 0) {
                    u32 f = (cfg >> 9) & 0x3FF;
                    GxBpWrite(((f | 0x98000000u) & ~0x3FC00u) | (((u32)*(u8*)(rec + 0x11) << 10) & 0x3FC00u));
                    GxBpWrite(0xFEFFFF00u);
                    GxBpWrite(0x0F000000u);
                    GxBpWrite(((*(u32*)(rec + 0x1C) >> 5) & 0xFFFFFF) | 0x64000000u);
                    GxBpWrite(((f | 0x65000000u) & ~0x1FC00u) |
                              ((((s32)*(u8*)(rec + 0xD) >> 1) << 10) & 0x1FC00u));
                    GxBpWrite(0xFEFFFF00u);
                    GxBpWrite(0x0F000000u);
                }
            }
            GxBpWrite(((u32)*(u16*)(rec + 0x4) - 1) | 0x30000000u);
            GxBpWrite(((u32)*(u16*)(rec + 0x6) - 1) | 0x31000000u);
            u8* buf;
            s32 bufSize;
            fn_80158530(tblA, tblB, &buf, &bufSize,
                        (f32)*(u16*)(rec + 0x0) / (f32)*(u16*)(rec + 0x4),
                        (f32)*(u16*)(rec + 0x2) / (f32)*(u16*)(rec + 0x6));
            for (s32 k = 0; k < 0x11; k++) {
                fn_80152D84(0x98, 0x40, bufSize, 0x40, buf + k * 0x300, 0, 0, 0);
            }
            GxBpWrite(0xFE000001);
            GxBpWrite(0x41000000);
        }
        rec += 0x20;
        tblB += 0x900;
        tblA += 0x1B00;
        enabled += 1;
    }
}

extern "C" {

// 0x80075ACC
// Scans the zone's object list and files each object into the level object's lookup slots/arrays by its
// type hash (spirit notes, keys rows, scoring, organ pipes, ghost, skullblast).
void fn_80075ACC(u8* p) {
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    s32 nPipe1 = 0;  // organ_pipeA
    s32 nPipe4 = 0;  // organ_pipe_04A
    s32 nPipe2 = 0;  // organ_pipe_02A
    s32 nPipe3 = 0;  // organ_pipe_03A
    for (s32 w = 0; w < *(s32*)(*(u8**)gWorldState + 0x4); w++) {
        for (s32 i = 0; i < *(s32*)(zone + 0x98); i++) {
            u8* obj = *(u8**)(*(u8**)(zone + 0x12C) + i * 4);
            u8* info = *(u8**)(obj + 0x9C);
            u32 hash = info ? *(u32*)(info + 0x8) : 0;
            s32 n;
            if (hash == kHashSpiritNote2) {
                n = *(s32*)(p + 0x574);
                *(u8**)(p + 0x430 + n * 4) = obj;
                *(s32*)(p + 0x574) = n + 1;
            } else if (hash == kHashSpiritNote) {
                n = *(s32*)(p + 0x570);
                *(u8**)(p + 0x3E0 + n * 4) = obj;
                *(s32*)(p + 0x570) = n + 1;
            } else if (hash == kHashSpiritNote3) {
                n = *(s32*)(p + 0x578);
                *(u8**)(p + 0x480 + n * 4) = obj;
                *(s32*)(p + 0x578) = n + 1;
            } else if (hash == kHashSpiritNote4) {
                n = *(s32*)(p + 0x57C);
                *(u8**)(p + 0x4D0 + n * 4) = obj;
                *(s32*)(p + 0x57C) = n + 1;
            } else if (hash == kHashSpiritNote5) {
                n = *(s32*)(p + 0x580);
                *(u8**)(p + 0x520 + n * 4) = obj;
                *(s32*)(p + 0x580) = n + 1;
            } else if (hash == kHashKeysRow1) {
                *(u8**)(p + 0x5A4) = obj;
            } else if (hash == kHashKeysRow2) {
                *(u8**)(p + 0x5AC) = obj;
            } else if (hash == kHashKeysRow3) {
                *(u8**)(p + 0x5B0) = obj;
            } else if (hash == kHashKeysRow4) {
                *(u8**)(p + 0x5A8) = obj;
            } else if (hash == kHashScoring) {
                *(u8**)(p + 0x584) = obj;
            } else if (hash == kHashScoring2) {
                *(u8**)(p + 0x58C) = obj;
            } else if (hash == kHashScoring3) {
                *(u8**)(p + 0x590) = obj;
            } else if (hash == kHashScoring4) {
                *(u8**)(p + 0x588) = obj;
            } else if (hash == kHashOrganPipe) {
                *(u8**)(p + 0x5B4 + nPipe1 * 0x10) = obj;
                nPipe1++;
            } else if (hash == kHashOrganPipe2) {
                *(u8**)(p + 0x5BC + nPipe2 * 0x10) = obj;
                nPipe2++;
            } else if (hash == kHashOrganPipe3) {
                *(u8**)(p + 0x5C0 + nPipe3 * 0x10) = obj;
                nPipe3++;
            } else if (hash == kHashOrganPipe4) {
                *(u8**)(p + 0x5B8 + nPipe4 * 0x10) = obj;
                nPipe4++;
            } else if (hash == kHashSpiritGhost) {
                *(u8**)(p + 0x5D4) = obj;
            } else if (hash == kHashSkullblast1) {
                *(u8**)(p + 0x594) = obj;
            } else if (hash == kHashSkullblast2) {
                *(u8**)(p + 0x59C) = obj;
            } else if (hash == kHashSkullblast3) {
                *(u8**)(p + 0x5A0) = obj;
            } else if (hash == kHashSkullblast4) {
                *(u8**)(p + 0x598) = obj;
            }
        }
    }
}

// 0x80077090
// Loads <name>.mid, <name>.wt and <name>.pcm into freshly allocated buffers (stored at +0x1234,
// +0x122C, +0x1230) and allocates a zeroed 0x100-byte work buffer at +0x1228.
void fn_80077090(u8* p, char* name) {
    char path[0x80];
    u8 file[0x60];

    strcpy(path, name);
    strcat(path, lbl_805F88F0);
    fn_80146808(file, path, lbl_805F88F8);
    s32 sz = fn_80146A58(file);
    *(void**)(p + 0x1234) = MemAllocSmall((u32)(sz * 2 - sz));
    fn_80146930(file, *(void**)(p + 0x1234), fn_80146A58(file));
    fn_801469FC(file);

    strcpy(path, name);
    strcat(path, lbl_805F88FC);
    fn_80146808(file, path, lbl_805F88F8);
    sz = fn_80146A58(file);
    *(void**)(p + 0x122C) = MemAllocSmall((u32)(sz * 2 - sz));
    fn_80146930(file, *(void**)(p + 0x122C), fn_80146A58(file));
    fn_801469FC(file);

    strcpy(path, name);
    strcat(path, lbl_805F8900);
    fn_80146808(file, path, lbl_805F88F8);
    sz = fn_80146A58(file);
    *(void**)(p + 0x1230) = MemAllocSmall((u32)(sz * 2 - sz));
    fn_80146930(file, *(void**)(p + 0x1230), fn_80146A58(file));
    fn_801469FC(file);

    u8* work = (u8*)MemAllocSmall(0x100);
    *(u8**)(p + 0x1228) = work;
    memset(work, 0, 0x100);
}

// 0x80087144
// Scans the zone's flagged objects (flag 0x40 in the record): picks out the castle brazier bowl/wand and the
// symbol/ball sets (up to 0x96 / 0x19 of them), initialises each and links them to the bowl and wand.
void fn_80087144(u8* p) {
    s32 nSymbols = 0;
    s32 nBadSymbols = 0;
    u8* symbolSlot = p;
    u8* badSlot = p;
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    s32 total = *(s32*)(zone + 0x94) + *(s32*)(zone + 0x98);
    s32 recOff = 0;
    s32 ptrOff = 0;
    for (s32 i = 0; i < total; i++) {
        zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
        s32 nFirst = *(s32*)(zone + 0x94);
        u8* obj;
        u8* rec;
        if (i < nFirst) {
            obj = *(u8**)(*(u8**)(zone + 0x120) + ptrOff);
            rec = *(u8**)(zone + 0x8C) + recOff;
        } else {
            s32 j = i - nFirst;
            obj = *(u8**)(*(u8**)(zone + 0x12C) + j * 4);
            rec = *(u8**)(zone + 0x90) + (j << 6);
        }
        if ((*(u32*)(rec + 0x1C) & 0x40) && obj != 0) {
            u8* info = *(u8**)(obj + 0x9C);
            u32 hash = info ? *(u32*)(info + 0x8) : 0;
            if (hash == kHashCastleBowl) {
                *(u8**)(p + 0x350) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                info = *(u8**)(obj + 0x9C);
                *(u32*)(*(u8**)(p + 0x350) + 0xF0) = info ? *(u32*)(info + 0x8) : 0;
                *(u32*)(*(u8**)(p + 0x350) + 0xF4) = 0x016A38A5;
                fn_8008CAA4(*(u8**)(p + 0x350));
            } else if (hash == kHashCastleWand) {
                *(u8**)(p + 0x354) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                info = *(u8**)(obj + 0x9C);
                *(u32*)(*(u8**)(p + 0x354) + 0xF0) = info ? *(u32*)(info + 0x8) : 0;
                *(u32*)(*(u8**)(p + 0x354) + 0xF4) = 0x016A38A5;
                fn_8008B850(*(u8**)(p + 0x354));
            } else if (hash == kHashCastleGood || hash == kHashCastleEarth || hash == kHashCastleFire ||
                       hash == kHashCastleAir || hash == kHashCastleWater) {
                if (nSymbols < 0x96) {
                    *(u8**)(symbolSlot + 0xF8) = obj;
                    *(u32*)(obj + 0x15C) |= 0x400000;
                    info = *(u8**)(obj + 0x9C);
                    *(u32*)(*(u8**)(symbolSlot + 0xF8) + 0xF0) = info ? *(u32*)(info + 0x8) : 0;
                    *(u32*)(*(u8**)(symbolSlot + 0xF8) + 0xF4) = 0x016A38A5;
                    fn_8008CCC4(*(u8**)(symbolSlot + 0xF8));
                    info = *(u8**)(obj + 0x9C);
                    fn_8008D6BC(*(u8**)(symbolSlot + 0xF8), info ? *(u32*)(info + 0x8) : 0);
                    symbolSlot += 4;
                    nSymbols++;
                }
            } else if (hash == kHashCastleBad) {
                if (nBadSymbols < 0x19) {
                    *(u8**)(badSlot + 0x94) = obj;
                    *(u32*)(obj + 0x15C) |= 0x400000;
                    info = *(u8**)(obj + 0x9C);
                    *(u32*)(*(u8**)(badSlot + 0x94) + 0xF0) = info ? *(u32*)(info + 0x8) : 0;
                    *(u32*)(*(u8**)(badSlot + 0x94) + 0xF4) = 0x016A38A5;
                    fn_8008CCC4(*(u8**)(badSlot + 0x94));
                    info = *(u8**)(obj + 0x9C);
                    fn_8008D6BC(*(u8**)(badSlot + 0x94), info ? *(u32*)(info + 0x8) : 0);
                    badSlot += 4;
                    nBadSymbols++;
                }
            } else {
                ScopedLock lock;
                *(u32*)(obj + 0x15C) |= 0x400000;
                ScopedLockCtor(&lock);
                (*(VCallFn*)(*(u8**)obj + 0x18))(obj, 1);
                ScopedLockDtor(&lock, -1);
            }
        }
        recOff += 0x40;
        ptrOff += 4;
    }
    fn_8008C8C8(*(u8**)(p + 0x354), *(u8**)(p + 0x368) + 8);
    for (s32 k = 0; k < 0x96; k++) {
        u8* sym = *(u8**)(p + 0xF8 + k * 4);
        if (sym != 0) {
            fn_8008D244(sym, *(u8**)(p + 0x354));
            fn_8008D24C(*(u8**)(p + 0xF8 + k * 4), *(u8**)(p + 0x350));
        }
    }
    for (s32 k = 0; k < 0x19; k++) {
        u8* sym = *(u8**)(p + 0x94 + k * 4);
        if (sym != 0) {
            fn_8008D244(sym, *(u8**)(p + 0x354));
            fn_8008D24C(*(u8**)(p + 0x94 + k * 4), *(u8**)(p + 0x350));
        }
    }
}

// 0x800946E0
// For each of the 27 slots: builds a record from the two name bytes, then picks (by random roll) which of
// the matching reactor objects (per team, 0x38D/0x38C counts) become the flagged one and links them up.
void fn_800946E0(u8* p) {
    u8* slot = p;
    u8* rec = p + 0x384;
    const u8* names = lbl_8035C1D0;
    for (s32 s = 0; s < 0x1B; s++) {
        u8 pair[2];
        pair[0] = names[0];
        pair[1] = names[1];
        fn_800975D0(rec, pair, *(u8**)(p + 0x90), s);
        u32 total = (u32)*(u8*)(slot + 0x38C) + (u32)*(u8*)(slot + 0x38D);
        s32 roll = (s32)(RandomU32() % total);
        for (s32 team = 0; team < 2; team++) {
            u8* dst = slot;
            for (s32 n = 0;; n++) {
                s32 limit;
                if (team == 1)      limit = *(u8*)(slot + 0x38C);
                else if (team == 0) limit = *(u8*)(slot + 0x38D);
                else                limit = -1;
                if (n >= limit) break;
                u8* found = 0;
                s32 cnt = *(s32*)(p + 0x1B4);
                for (s32 k = 0; k < cnt; k++) {
                    u8* cand = *(u8**)(p + 0x9C + k * 4);
                    if (team == (*(VQueryFn*)(*(u8**)cand + 0xBC))(cand) &&
                        *(s32*)(*(u8**)(p + 0x9C + k * 4) + 0x268) < 0) {
                        found = *(u8**)(p + 0x9C + k * 4);
                        break;
                    }
                }
                if (team == 1)      *(u8**)(dst + 0x398) = found;
                else if (team == 0) *(u8**)(dst + 0x390) = found;
                fn_80097A18(found, *(u8**)(p + 0x90), s, n);
                *(u8*)(found + 0x26E) = (u8)(roll == 0);
                roll--;
                dst += 4;
            }
        }
        rec += 0x20;
        slot += 0x20;
        names += 2;
    }
}

// 0x8009487C
// Scans the zone's flagged objects and registers the reactor parts: heart, hammer, screwdriver, bolts
// (<= 0x22), screws (<= 0x1C), lightning, and the light objects; then initialises the collected objects.
void fn_8009487C(u8* p) {
    u8* lightning = 0;
    *(s32*)(p + 0x1B4) = 0;
    u8* lightSlot = p;
    s32 nBolts = 0;
    s32 nScrews = 0;
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
    s32 total = *(s32*)(zone + 0x94) + *(s32*)(zone + 0x98);
    s32 recOff = 0;
    s32 ptrOff = 0;
    for (s32 i = 0; i < total; i++) {
        zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + gZoneResourceSize;
        s32 nFirst = *(s32*)(zone + 0x94);
        u8* obj;
        u8* rec;
        if (i < nFirst) {
            obj = *(u8**)(*(u8**)(zone + 0x120) + ptrOff);
            rec = *(u8**)(zone + 0x8C) + recOff;
        } else {
            s32 j = i - nFirst;
            obj = *(u8**)(*(u8**)(zone + 0x12C) + j * 4);
            rec = *(u8**)(zone + 0x90) + (j << 6);
        }
        if ((*(u32*)(rec + 0x1C) & 0x40) && obj != 0) {
            u8* info = *(u8**)(obj + 0x9C);
            u32 hash = info ? *(u32*)(info + 0x8) : 0;
            if (hash == kHashReactorHeart) {
                *(u8**)(p + 0x90) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                fn_80098E74(*(u8**)(p + 0x90));
            } else if (hash == kHashReactorHammer) {
                *(u8**)(p + 0x94) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                *(u32*)(*(u8**)(p + 0x94) + 0x15C) |= 0x200000;
                u8* h = *(u8**)(p + 0x94);
                (*(VQueryFn*)(*(u8**)h + 0xBC))(h);
            } else if (hash == kHashReactorScrewdrv) {
                *(u8**)(p + 0x98) = obj;
                *(u32*)(obj + 0x15C) |= 0x400000;
                u8* sd = *(u8**)(p + 0x98);
                ScopedLock lock;
                ScopedLockCtor(&lock);
                (*(VCallFn*)(*(u8**)sd + 0x18))(sd, 1);
                ScopedLockDtor(&lock, -1);
            } else if (hash == kHashReactorBolts) {
                ScopedLock lock;
                *(u32*)(obj + 0x15C) |= 0x400000;
                ScopedLockCtor(&lock);
                (*(VCallFn*)(*(u8**)obj + 0x18))(obj, 1);
                ScopedLockDtor(&lock, -1);
                if (nBolts < 0x22) {
                    nBolts++;
                    *(u8**)(p + 0x9C + *(s32*)(p + 0x1B4) * 4) = obj;
                    *(s32*)(p + 0x1B4) = *(s32*)(p + 0x1B4) + 1;
                }
            } else if (hash == kHashReactorScrews) {
                ScopedLock lock;
                *(u32*)(obj + 0x15C) |= 0x400000;
                ScopedLockCtor(&lock);
                (*(VCallFn*)(*(u8**)obj + 0x18))(obj, 1);
                ScopedLockDtor(&lock, -1);
                if (nScrews < 0x1C) {
                    nScrews++;
                    *(u8**)(p + 0x9C + *(s32*)(p + 0x1B4) * 4) = obj;
                    *(s32*)(p + 0x1B4) = *(s32*)(p + 0x1B4) + 1;
                }
            } else if (hash == kHashReactorLights || hash == kHashReactorLightsAlt) {
                *(u32*)(obj + 0x15C) |= 0x400000;
                *(u8**)(lightSlot + 0x194) = obj;
                fn_80098700(obj);
                lightSlot += 4;
            } else if (hash == kHashReactorLightning) {
                ScopedLock lock;
                lightning = obj;
                ScopedLockCtor(&lock);
                (*(VCallFn*)(*(u8**)obj + 0x18))(obj, 1);
                ScopedLockDtor(&lock, -1);
            }
        }
        recOff += 0x40;
        ptrOff += 4;
    }
    *(u8**)(*(u8**)(p + 0x90) + 0x264) = lightning;
    for (s32 k = 0; k < *(s32*)(p + 0x1B4); k++) {
        fn_800980F8(*(u8**)(p + 0x9C + k * 4), 0);
        fn_800980C8(*(u8**)(p + 0x9C + k * 4), 0);
    }
}

// 0x80138A88
// Advances a path segment: when the interpolation factor is outside [a+0x18, 1], and the mask check passes,
// normalises the direction vector (falling back to +Z if it is ~zero) and recomputes the position at the
// segment's t. Always returns 1.
s32 fn_80138A88(u8* a, u8* b) {
    f32 t = *(f32*)(b + 0x18);
    if (t > lbl_805FEF68 || !(*(f32*)(a + 0x18) >= t)) {
        u32 bits = **(u32**)(a + 0x1C);
        u32 mask = (bits >> 16) & 0xF;
        *(u32*)(b + 0x48) = bits;
        if (!(!(*(u32*)(b + 0x50) & ~mask) && (*(u32*)(b + 0x54) & mask))) {
            return 1;
        }
        f32* v = *(f32**)(a + 0x20);
        f32 dx = v[0] - lbl_805FEF6C;
        f32 dy = v[1] - lbl_805FEF6C;
        f32 dz = v[2] - lbl_805FEF6C;
        if (dx <= lbl_805FEF70 && dx >= lbl_805FEF74 && dy <= lbl_805FEF70 && dy >= lbl_805FEF74 &&
            dz <= lbl_805FEF70 && dz >= lbl_805FEF74) {
            v[0] = lbl_805FEF6C;
            v[1] = lbl_805FEF6C;
            v[2] = lbl_805FEF68;
        }
        f32* v2 = *(f32**)(a + 0x20);
        *(f32*)(b + 0xC) = v2[0];
        *(f32*)(b + 0x10) = v2[1];
        *(f32*)(b + 0x14) = v2[2];
        f32 x0 = *(f32*)(a + 0x0);
        f32 y0 = *(f32*)(a + 0x4);
        f32 z0 = *(f32*)(a + 0x8);
        f32 ex = *(f32*)(a + 0xC) - x0;
        f32 ey = *(f32*)(a + 0x10) - y0;
        f32 ez = *(f32*)(a + 0x14) - z0;
        f32 s = *(f32*)(a + 0x18);
        *(f32*)(b + 0x0) = x0 + ex * s;
        *(f32*)(b + 0x4) = y0 + ey * s;
        *(f32*)(b + 0x8) = z0 + ez * s;
        *(f32*)(b + 0x18) = *(f32*)(a + 0x18);
    }
    return 1;
}

// 0x801588D8
// Sets up the GX state for the screen-capture pass (BP register writes), copies the capture regions
// for both buffers, and finishes with the display-list flush calls.
void fn_801588D8(void) {
    u8* buf;
    s32 bufSize;
    s32 tbl[6];

    fn_80158BC4(0x7800);
    fn_8014950C(0, 0);
    // The original passes &lbl_805FB598 as r3 to the fn_80158xxx BP helpers; they ignore it.
    fn_8015846C(0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    fn_80158C34(0, 0, 0, 1, 1, 0, 0, 0, 0, 7);
    fn_80158C94(0, 0, 0xF, 8, 0xA, 0xF, 0, 0, 0, 1, 0);
    fn_80158D58(0, 0, 7, 7, 7, 5, 0, 0, 0, 1, 0, 0, 0);
    fn_80158E64(0, 1);
    fn_80158E88(0, 0, 0, 0, 0, 1, 6, 6, 0, 0, 0);
    for (s32 i = 0; i < 6; i++) tbl[i] = lbl_80348938[i];
    fn_80158F04(0, 0, tbl, 1);
    fn_80158FBC(0, 0, 0, 0, 0, 0);
    fn_80158FF4(0, 0, 0, 0, 0, 0, 0, 0, 0);
    fn_80158530(lbl_80401C40, lbl_80403740, &buf, &bufSize, lbl_805FF1BC, lbl_805FF1BC);
    s32 flip = lbl_805FB518 & 1;
    fn_80158228(lbl_8040AC50, 0, 0);
    fn_801584E8(lbl_8040AC50, 0);
    u8* alt = lbl_8040ACB0 + flip * 0x60;
    fn_80158228(alt, 1, 1);
    fn_80158870(buf, bufSize, 0, 0, 0, 0, 0, 0);
    fn_80044C0C();
    fn_80159058();
    fn_80158228(lbl_8040AC50 + 0x20, 0, 0);
    fn_801584E8(lbl_8040AC50 + 0x20, 0);
    fn_80158228(alt + 0x20, 1, 1);
    fn_80158870(buf, bufSize, 0, 0, 0, 0, 0, 0);
    fn_80158228(lbl_8040AC50 + 0x40, 0, 0);
    fn_801584E8(lbl_8040AC50 + 0x40, 0);
    fn_80158228(alt + 0x40, 1, 1);
    fn_80158870(buf, bufSize, 0, 0, 0, 0, 0, 0);
    fn_80158E64(0, 0);
    fn_80159084(0, 0);
    fn_80149594();
    fn_801590A0();
}

// 0x801598AC
// Renders the three capture layers, then finishes the three header blocks and the blur block with mode 0.
void fn_801598AC(void) {
    RenderCaptureLayers();
    fn_801578D8(lbl_80401C30 + 0x10000 - 0x6FE0, 0x30008, 0);
    u8* hdr = lbl_80401C30 + 0x10000 - 0x6FE0;
    fn_801578D8(hdr + 0x20, 0x30018, 0);
    fn_801578D8(hdr + 0x40, 0x30028, 0);
    fn_80157B28(lbl_80401C30 + 0x10000 - 0x6EC0, 0x10060, 0);
}

// 0x80159C60
// Same as fn_801598AC but the finishing blocks use mode 1 when flag != 0, else 2.
void fn_80159C60(s32 flag) {
    RenderCaptureLayers();
    s32 mode = flag != 0 ? 1 : 2;
    fn_801578D8(lbl_80401C30 + 0x10000 - 0x6FE0, 0x30008, mode);
    u8* hdr = lbl_80401C30 + 0x10000 - 0x6FE0;
    fn_801578D8(hdr + 0x20, 0x30018, mode);
    fn_801578D8(hdr + 0x40, 0x30028, mode);
    fn_80157B28(lbl_80401C30 + 0x10000 - 0x6EC0, 0x10060, mode);
}

// 0x801601DC
// Initialises a camera/light record: copies the 10 floats at +0x78 to +0x118, the bytes at +0/+1 and 29
// floats at +4..+0x74 to +0xA0/+0xA1/+0xA4.., and sets the default state fields.
void fn_801601DC(u8* p) {
    fn_8015EB3C(p);
    for (s32 i = 0; i < 10; i++) {
        *(f32*)(p + 0x118 + i * 4) = *(f32*)(p + 0x78 + i * 4);
    }
    *(u8*)(p + 0xA0) = *(u8*)(p + 0x0);
    *(u8*)(p + 0xA1) = *(u8*)(p + 0x1);
    for (s32 i = 0; i < 29; i++) {
        *(f32*)(p + 0xA4 + i * 4) = *(f32*)(p + 0x4 + i * 4);
    }
    *(f32*)(p + 0x14C) = lbl_805FF228;
    *(f32*)(p + 0x150) = lbl_805FF228;
    *(s32*)(p + 0x140) = 1;
    *(f32*)(p + 0x154) = lbl_805FF250;
    *(s32*)(p + 0x144) = 0;
    *(f32*)(p + 0x158) = lbl_805FF22C;
}

// 0x8016DDAC
// Rotates the vector at +0x18 by the quaternion built from the axis at +0/+4/+8 (angle lbl_805FF604) and
// writes the result into the X/Y/Z fields at +0x38/+0x3C/+0x40 of the object at lbl_805FB5DC.
void fn_8016DDAC(u8* p) {
    Vec3 axis;
    Quat q;
    f32 vx = *(f32*)(p + 0x18);
    f32 vy = *(f32*)(p + 0x1C);
    f32 vz = *(f32*)(p + 0x20);
    axis.x = *(f32*)(p + 0x0);
    axis.y = *(f32*)(p + 0x4);
    axis.z = *(f32*)(p + 0x8);
    QuatFromAxisAngle(&q, &axis, lbl_805FF604);
    f32 z0 = lbl_805FF5E8;
    f32 nx = -q.x;
    f32 nz = -q.z;
    f32 ny = -q.y;
    // t = q * (0, v)
    f32 tx = ((q.y * vz) + ((q.w * vx) + (q.x * z0))) - (q.z * vy);
    f32 tw = (((q.w * z0) - (q.x * vx)) - (q.y * vy)) - (q.z * vz);
    f32 ty = (q.z * vx) + ((q.y * z0) + ((q.w * vy) - (q.x * vz)));
    f32 tz = (q.z * z0) + (((q.w * vz) + (q.x * vy)) - (q.y * vx));
    // r = t * conj(q)
    f32 rx = ((ty * nz) + ((tw * nx) + (tx * q.w))) - (tz * ny);
    f32 ry = (tz * nx) + ((ty * q.w) + ((tw * ny) - (tx * nz)));
    f32 rz = (tz * q.w) + (((tw * nz) + (tx * ny)) - (ty * nx));
    *(f32*)((u8*)lbl_805FB5DC + 0x38) = rx;
    *(f32*)((u8*)lbl_805FB5DC + 0x3C) = ry;
    *(f32*)((u8*)lbl_805FB5DC + 0x40) = rz;
}

}  // extern "C"
