// Misc decompiled functions (batch miss143): zone-slot refresh, world segment query, matchup/options
// per-frame timers, axis-angle matrix, basis orthonormalisation, 4bpp texture upload, GX viewport
// setup and render-target palette tables.
#include <math.h>

#include "types.h"
#include "engine/math.h"
#include "engine/rend.h"
#include "revolution.h"

extern "C" {
#include "game/script_callbacks.h"
}

// C++ linkage, as in small_8015.cpp which defines them.
void fn_8015A2E8(u8* p, u32 unused4, u32 flush);                       // 0x8015A2E8
void fn_8015A368(u8* p, u32 flush);                                    // 0x8015A368

extern "C" {
u8*   GetGlobalSettings(void);
void  MatchupSetReady(BOOL ready);
void* ScreenSetFindWidget(void* set, u32 hash);                       // 0x80198E34
void  Vec3Copy(Vec3* out, const Vec3* v);
void  Vec3Dot(f32* out, const Vec3* a, const Vec3* b);
BOOL  ScalarLess(const f32* a, const f32* b);
void  ScalarZero(f32* out);
s32   VideoIsPal50(RendVideo* video);                                 // 0x8015DE24
void  fn_8005A244(Vec3* out, const Vec3* a, const Vec3* b);           // 0x8005A244 (cross)
BOOL  fn_80058BE8(const f32* a, const f32* b);                        // 0x80058BE8 (a < b)
void  fn_800636A0(void);                                              // 0x800636A0 (no-op scalar ctor)
void  fn_800676E0(Vec3* out, const Vec3* v, f32* len);                // 0x800676E0 (normalize, length out)
void  fn_801384B0(u8* world, s32 slot, s32 a);                        // 0x801384B0
s32   fn_80138A88(u8* hit, u8* query);                                // 0x80138A88 (query callback)
void  fn_8014210C(void);                                              // 0x8014210C
BOOL  fn_80142358(void);                                              // 0x80142358
void  fn_80142ED0(u8* base, s32 size);                                // 0x80142ED0
void  fn_80144F78(u8* out, u8* in);                                   // 0x80144F78 (fabs)
BOOL  fn_80147554(s32 button);                                        // 0x80147554
void  fn_80149E98(u8* dst, s32 fmt, u8* src, s32 type, s32 count);    // 0x80149E98
u8*   fn_8015A210(u8* p, s32 a, s32* out, s32 b);                     // 0x8015A210
u8*   fn_8015A340(u8* p, s32 a);                                      // 0x8015A340
void  fn_8015A5A4(u8* p, s32 a);                                      // 0x8015A5A4
void  fn_8015B7D4(void);                                              // 0x8015B7D4
s32   fn_8015BA30(void* p);                                           // 0x8015BA30
void  fn_8016BB04(u8* shape, f32* a, f32* b, void* cb, u8* q);        // 0x8016BB04
void  fn_8016C2AC(u8* shape, f32* a, f32* b, void* cb, u8* q);        // 0x8016C2AC
BOOL  fn_80198C34(void* set, void* w);                                // 0x80198C34
s32   fn_801D3A60(s32 zone);                                          // 0x801D3A60
void  fn_801F33C8(f32 a, f32 b);                                      // 0x801F33C8
BOOL  fn_801F56FC(s32 flags);                                         // 0x801F56FC
void  fn_801F5714(s32 flags);                                         // 0x801F5714
void  fn_801F5724(u32 button);                                        // 0x801F5724
s32   fn_801FCA38(f32 x, f32 y);                                      // 0x801FCA38
void  RendSetDefaultState(void);

extern u8      gWorldState[];       // 0x803E6D00
extern u8      sRendDLs[];          // 0x80401108 (display-list buffers, 0x94 bytes each)
extern u8      sRendDLRecords[];    // 0x80401ADC (display-list records, 0x14 bytes each)
extern u8      lbl_80348908[];      // 0x80348908 (GX register table, 0x18 bytes per entry)
extern u8      lbl_80401C30[];      // 0x80401C30 (three pointers to 256-entry colour tables)
extern u8      lbl_8040ACB0[];      // 0x8040ACB0
extern s32     lbl_805F9214;        // 0x805F9214
extern s32     lbl_805FB30C;        // 0x805FB30C (world object count)
extern f32     lbl_805FB410;        // 0x805FB410 (matchup countdown)
extern u8      lbl_805FB414;        // 0x805FB414 (matchup ready flag)
extern f32     lbl_805FB424;        // 0x805FB424
extern f32     lbl_805FB428;        // 0x805FB428
extern s32     lbl_805FB42C;        // 0x805FB42C
extern u8      lbl_805FB43C;        // 0x805FB43C
extern s32     lbl_805FB4BC;        // 0x805FB4BC (current display list)
extern s32     lbl_805FB4C0;        // 0x805FB4C0
extern s32     lbl_805FB4C4;        // 0x805FB4C4
extern s32     lbl_805FB4C8;        // 0x805FB4C8
extern s32     lbl_805FB4CC;        // 0x805FB4CC
extern u8*     lbl_805FB4D0;        // 0x805FB4D0
extern s32     lbl_805FB4D4;        // 0x805FB4D4
extern s32     lbl_805FB4D8;        // 0x805FB4D8
extern s32     lbl_805FB4DC;        // 0x805FB4DC
extern s32     lbl_805FB4E0;        // 0x805FB4E0
extern s32     lbl_805FB4E8;        // 0x805FB4E8
extern s32     lbl_805FB4EC;        // 0x805FB4EC
extern s32     lbl_805FB508;        // 0x805FB508
extern u8      lbl_805FB50C;        // 0x805FB50C (GX shadow flags byte)
extern u8      lbl_805FB50D;        // 0x805FB50D
extern s32     lbl_805FB514;        // 0x805FB514 (GX shadow word)
extern u8      lbl_805FB518[];      // 0x805FB518
extern f32     lbl_805FB918;        // 0x805FB918 (frame time step)
extern f32     lbl_805FB91C;        // 0x805FB91C
extern void*   lbl_805FB964;        // 0x805FB964 (movie player)
extern s32     lbl_805FBA30;        // 0x805FBA30 (home button icon state)
extern f32     lbl_805FBA80;        // 0x805FBA80
extern f32     lbl_806002D8;        // 0x806002D8
}

extern "C++" void* gScreenSets;     // 0x805FB654

static const u32 kHashMultiplayerScreen = 0x8D7C9EE0u;  // "UI_MSA_Multiplayer"
static const u32 kHashWidget = 0x28DCC9E9u;             // widget on the multiplayer screen
static const u32 kEventMatchupTimeout = 0x11A4EB2Bu;

static inline s32 GS(s32 off) { return *(s32*)(GetGlobalSettings() + off); }

// True when the button or pad combination that cancels the matchup countdown is pressed.
static inline BOOL MatchupCancelPressed(void) {
    if (fn_801F56FC(0x10) != 0 && fn_80142358() != 0) return 1;
    if ((fn_801F56FC(4) != 0 || fn_801F56FC(8) != 0) && fn_80142358() != 0) return 1;
    return 0;
}

extern "C" {

// 0x80133840
// If the slot index lies in the last settings range (after the first eight zone groups and up to
// the ninth), refreshes the five zone slots starting at zone group 1.
void fn_80133840(u8* world, s32 index) {
    s32 sum = GS(0x6C);
    sum = GS(0x68) + sum;
    sum = sum + GS(0x70);
    sum = sum + GS(0x74);
    sum = sum + GS(0x78);
    sum = sum + GS(0x7C);
    sum = sum + GS(0x80);
    sum = sum + GS(0x84);
    if (sum + 1 > index) return;

    s32 a = GS(0x68);
    s32 b = GS(0x88);
    s32 c = GS(0x6C);
    b = b + a;
    s32 d = GS(0x70);
    b = c + b;
    s32 e = GS(0x74);
    b = d + b;
    s32 f = GS(0x78);
    e = e + b;
    s32 g = GS(0x7C);
    f = f + e;
    s32 h = GS(0x80);
    f = g + f;
    s32 i = GS(0x84);
    if (index > i + (h + f)) return;

    fn_801384B0(world, fn_801D3A60(GS(0x68) + 1), 1);
    s32 t = GS(0x6C);
    fn_801384B0(world, fn_801D3A60(GS(0x68) + t + 1), 1);
    t = GS(0x6C);
    t = GS(0x68) + t;
    fn_801384B0(world, fn_801D3A60(t + GS(0x70) + 1), 1);
    t = GS(0x6C);
    t = GS(0x68) + t;
    t = t + GS(0x70);
    fn_801384B0(world, fn_801D3A60(t + GS(0x74) + 1), 1);
    t = GS(0x6C);
    t = GS(0x68) + t;
    t = t + GS(0x70);
    t = t + GS(0x74);
    fn_801384B0(world, fn_801D3A60(t + GS(0x78) + 1), 1);
}

// 0x80138C40
// Segment query from p0 to p1 against the collision shapes of the active world objects (type 6
// slots within the settings range). Fills `out` (if given) with the nearest hit record and returns
// whether anything was hit. arg3 supplies the filter masks at +0x50/+0x54.
s32 fn_80138C40(u8* unused0, f32* p0, f32* p1, u8* out, u8* unused4, s32 flag, s32 unused6) {
    u8 q[0x6C];
    s32 hit = 0;
    f32 dx = p0[0] - p1[0];
    f32 dy = p0[1] - p1[1];
    f32 dz = p0[2] - p1[2];

    for (s32 k = 0; k < 0x6C; k++) q[k] = 0;
    *(s32*)(q + 0x50) = *(s32*)(out + 0x50);
    *(s32*)(q + 0x54) = *(s32*)(out + 0x54);
    *(f32*)(q + 0x18) = 1.1f;

    if (dx <= 0.0001f && dx >= -0.0001f && dy <= 0.0001f && dy >= -0.0001f &&
        dz <= 0.0001f && dz >= -0.0001f) {
        return 0;
    }

    for (s32 i = 0; i < lbl_805FB30C; i++) {
        u8* obj = (*(u8***)(gWorldState + 0xD4))[i];
        if (obj == NULL) continue;
        BOOL inRange = 0;
        if (i >= 0) {
            s32 sum = GS(0x4C);
            sum = GS(0x34) + sum;
            sum = GS(0x50) + sum;
            sum = GS(0x54) + sum;
            sum = GS(0x58) + sum;
            sum = GS(0x5C) + sum;
            sum = GS(0x60) + sum;
            sum = GS(0x64) + sum;
            sum = GS(0x38) + sum;
            sum = GS(0x3C) + sum;
            sum = GS(0x40) + sum;
            sum = GS(0x44) + sum;
            sum = GS(0x48) + sum;
            if (i < sum) inRange = 1;
        }
        BOOL isType6 = inRange ? (*(s32*)(*(u8**)(gWorldState + 0xB0) + i * 0x30 + 4) == 6) : 0;
        if (!isType6) continue;

        obj = (*(u8***)(gWorldState + 0xD4))[i];
        u8* shape = *(u8**)(obj + 0x4C);
        if (*(s32*)(shape + 0x1C) == 0) continue;
        *(u8**)(q + 0x4C) = obj;
        if (flag != 0) {
            fn_8016C2AC(*(u8**)(shape + 0x1C), p0, p1, (void*)fn_80138A88, q);
        } else {
            fn_8016BB04(*(u8**)(shape + 0x1C), p0, p1, (void*)fn_80138A88, q);
        }
        if (*(f32*)(q + 0x18) < 1.1f) {
            hit = 1;
            if (out != NULL) {
                *(s32*)(out + 0x48) = *(s32*)(q + 0x48);
                *(f32*)(out + 0x00) = *(f32*)(q + 0x00);
                *(f32*)(out + 0x04) = *(f32*)(q + 0x04);
                *(f32*)(out + 0x08) = *(f32*)(q + 0x08);
                *(f32*)(out + 0x0C) = *(f32*)(q + 0x0C);
                *(f32*)(out + 0x10) = *(f32*)(q + 0x10);
                *(f32*)(out + 0x14) = *(f32*)(q + 0x14);
                *(f32*)(out + 0x18) = *(f32*)(q + 0x18);
            }
        }
    }
    return hit;
}

// 0x8013F160
// Per-frame timers: the matchup cleanup countdown (runs MatchupCleanup at zero) and the second
// countdown that raises the matchup timeout event unless an error or the cancel input is active.
void fn_8013F160(void) {
    fn_801F33C8(lbl_805FB918, lbl_805FB91C);
    fn_8014210C();
    if (lbl_805FB424 > 0.0f) {
        f32 t = lbl_805FB424 - lbl_805FB918;
        lbl_805FB424 = t;
        if (t <= 0.0f) {
            lbl_805FB424 = 0.0f;
            MatchupCleanup();
        }
    }
    if (lbl_805FB428 > 0.0f) {
        if (IsShowingError() == 0 && !MatchupCancelPressed()) {
            f32 t = lbl_805FB428 - lbl_805FB918;
            lbl_805FB428 = t;
            if (t <= 0.0f) {
                lbl_805FB428 = 0.0f;
                GameFlowSignal((s32)kEventMatchupTimeout, 0, 0);
            }
        } else {
            lbl_805FB428 = 0.0f;
        }
    }
}

// 0x80140D3C
// Per-frame options/multiplayer screen update: saves pending option changes, tracks the
// multiplayer screen's widget state, and runs the matchup-ready countdown.
void fn_80140D3C(void) {
    if (lbl_805FB42C == 0) return;

    if (lbl_805FB964 != NULL && *(u8*)((u8*)lbl_805FB964 + 0xB1) != 0) {
        SaveManager_SaveOptions();
        *(u8*)((u8*)lbl_805FB964 + 0xB1) = 0;
    }
    void* set = *(void**)((u8*)gScreenSets + 0x88);
    if (set != NULL && *(u32*)set == kHashMultiplayerScreen) {
        fn_801F5724(0x20);
        void* w = ScreenSetFindWidget(set, kHashWidget);
        if (w != NULL) {
            lbl_805FB43C = (u8)fn_80198C34(set, w);
        }
    } else {
        fn_801F5714(0x20);
        lbl_805FB43C = 0;
    }
    if (lbl_805FBA30 == 1 && fn_80147554(0x108B) != 0) {
        fn_801FCA38(lbl_805FBA80, lbl_806002D8);
    }
    if (IsShowingError() != 0 || MatchupCancelPressed()) {
        if (lbl_805FB414 != 0) {
            MatchupSetReady(0);
        }
    }
    if (lbl_805FB414 != 0 && lbl_805FB410 > 0.0f) {
        f32 t = lbl_805FB410 - lbl_805FB918;
        lbl_805FB410 = t;
        if (t <= 0.0f) {
            lbl_805FB410 = 0.0f;
        }
    }
}

// 0x80144624
// Builds a rotation matrix (3x3 rows of 4 floats, last column and last row zero) for a rotation of
// `angle` quarter-turns around the unit axis.
void fn_80144624(u8* mtx, const Vec3* axis, f32 angle) {
    f32 t = 1.5707964f * angle;
    f32 c = (f32)cosf(t);
    f32 s = (f32)sinf(t);
    f32 x = axis->x;
    f32 z = axis->z;
    f32 omc = 1.0f - c;
    f32 y = axis->y;
    *(f32*)(mtx + 0x24) = 0.0f;
    *(f32*)(mtx + 0x28) = 0.0f;
    *(f32*)(mtx + 0x2C) = 0.0f;
    f32 xy = omc * (x * y);
    *(f32*)(mtx + 0x20) = c + z * z * omc;
    f32 sz = s * z;
    f32 xz = omc * (x * z);
    *(f32*)(mtx + 0x00) = c + x * x * omc;
    f32 sy = s * y;
    *(f32*)(mtx + 0x10) = c + y * y * omc;
    *(f32*)(mtx + 0x04) = xy + sz;
    f32 yz = omc * (y * z);
    *(f32*)(mtx + 0x0C) = xy - sz;
    f32 sx = s * x;
    *(f32*)(mtx + 0x08) = xz - sy;
    *(f32*)(mtx + 0x18) = xz + sy;
    *(f32*)(mtx + 0x14) = yz + sx;
    *(f32*)(mtx + 0x1C) = yz - sx;
}

// 0x80144C60
// Normalises three basis vectors (at +0, +0xC, +0x18) and re-orthogonalises them with cross
// products, keeping the two best-aligned axes: the axis with the smallest |dot of the other two|
// is rebuilt last. Writes the result back.
void fn_80144C60(f32* basis) {
    Vec3 v0, v1, v2, tmp;
    f32 len0, len1, len2;
    f32 z0, z1, z2;
    f32 d0, d1, d2;
    f32 a0, a1, a2;

    fn_800636A0();
    fn_800636A0();
    fn_800636A0();
    fn_800676E0(&v0, (Vec3*)basis, &len0);
    fn_800676E0(&v1, (Vec3*)(basis + 3), &len1);
    fn_800676E0(&v2, (Vec3*)(basis + 6), &len2);
    ScalarZero(&z0);
    if (fn_80058BE8(&len0, &z0) != 0) {
        ScalarZero(&z1);
        if (fn_80058BE8(&len1, &z1) != 0) {
            ScalarZero(&z2);
            if (fn_80058BE8(&len2, &z2) != 0) {
                Vec3Dot(&d0, &v1, &v2);
                fn_80144F78((u8*)&a0, (u8*)&d0);
                Vec3Dot(&d1, &v2, &v0);
                fn_80144F78((u8*)&a1, (u8*)&d1);
                Vec3Dot(&d2, &v0, &v1);
                fn_80144F78((u8*)&a2, (u8*)&d2);
                if (ScalarLess(&a0, &a1) != 0) {
                    if (ScalarLess(&a0, &a2) != 0) {
                        fn_8005A244(&tmp, &v1, &v2);
                        Vec3Copy(&v0, &tmp);
                        fn_8005A244(&tmp, &v0, &v1);
                        Vec3Copy(&v2, &tmp);
                    } else {
                        fn_8005A244(&tmp, &v0, &v1);
                        Vec3Copy(&v2, &tmp);
                        fn_8005A244(&tmp, &v2, &v0);
                        Vec3Copy(&v1, &tmp);
                    }
                } else if (ScalarLess(&a1, &a2) != 0) {
                    fn_8005A244(&tmp, &v2, &v0);
                    Vec3Copy(&v1, &tmp);
                    fn_8005A244(&tmp, &v1, &v2);
                    Vec3Copy(&v0, &tmp);
                } else {
                    fn_8005A244(&tmp, &v0, &v1);
                    Vec3Copy(&v2, &tmp);
                    fn_8005A244(&tmp, &v2, &v0);
                    Vec3Copy(&v1, &tmp);
                }
            } else {
                fn_8005A244(&tmp, &v0, &v1);
                Vec3Copy(&v2, &tmp);
                fn_8005A244(&tmp, &v2, &v0);
                Vec3Copy(&v1, &tmp);
            }
        } else {
            fn_8005A244(&tmp, &v2, &v0);
            Vec3Copy(&v1, &tmp);
            fn_8005A244(&tmp, &v1, &v2);
            Vec3Copy(&v0, &tmp);
        }
    } else {
        fn_8005A244(&tmp, &v1, &v2);
        Vec3Copy(&v0, &tmp);
        fn_8005A244(&tmp, &v0, &v1);
        Vec3Copy(&v2, &tmp);
    }
    Vec3Copy((Vec3*)basis, &v0);
    Vec3Copy((Vec3*)(basis + 3), &v1);
    Vec3Copy((Vec3*)(basis + 6), &v2);
}

// 0x80149B4C
// Initialises a 4bpp texture header (width x height) and fills it from `src` with the two nibbles
// of every byte swapped, row by row; `palette` is uploaded as the 16-entry palette.
void fn_80149B4C(u8* tex, u8* src, u8* palette, s32 width, s32 height, s32 fmt) {
    s32 stride;
    *(u8*)(tex + 0xB) = 0;
    *(u8*)(tex + 0x8) = 0;
    *(u8*)(tex + 0x9) = 0xB;
    *(u8*)(tex + 0xA) = 4;
    *(u8*)(tex + 0xC) = 6;
    *(u8*)(tex + 0xD) = 2;
    *(u8*)(tex + 0xE) = 0x10;
    *(u16*)(tex + 0x0) = (u16)width;
    *(u16*)(tex + 0x2) = (u16)height;
    fn_8015A5A4(tex, fmt);
    *(u8*)(tex + 0xB) = (u8)(*(u8*)(tex + 0xB) | 1);
    stride = 0;
    u8* pal = fn_8015A340(tex, 2);
    fn_80149E98(pal, 6, palette, 3, 0x10);
    fn_8015A368(tex, 1);
    u8* dst = fn_8015A210(tex, 0, &stride, 6);
    s32 half = (s32)(((u32)width >> 31) + width) >> 1;
    for (s32 y = 0; y < height; y++) {
        u8* d = dst;
        for (s32 j = 0; j < half; j++) {
            u8 b = *src++;
            *d++ = (u8)((b << 4) | (b >> 4));
        }
        dst = dst + stride;
    }
    fn_8015A2E8(tex, 0, 3);
}

// 0x8014ED64
// Sets up the GX state for a field-rendered pass (arg0 selects the odd/even field): copy-filter
// registers from the current table entry, a 2D position matrix scaled to the video size, and the
// scissor / copy-clear registers for the field.
void fn_8014ED64(s32 field) {
    s32 pal = VideoIsPal50(&gVideo);
    s32 width = VideoGetWidth(&gVideo);
    s32 height = VideoGetHeight(&gVideo);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x410004BDu;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x211;
    GXWGFifo._u8 = 0x61;
    u8* e = lbl_80348908 + (lbl_805F9214 - 1) * 0x18;
    GXWGFifo._u32 = ((u32)e[4] << 16) | ((u32)e[3] << 12) | (((u32)e[2] << 8) | (e[0] | ((u32)e[1] << 4))) |
                    (((u32)e[5] << 20) | 0x01000000u);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = ((u32)e[10] << 16) | ((u32)e[9] << 12) | (((u32)e[8] << 8) | (e[6] | ((u32)e[7] << 4))) |
                    (((u32)e[11] << 20) | 0x02000000u);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = ((u32)e[16] << 16) | ((u32)e[15] << 12) | (((u32)e[14] << 8) | (e[12] | ((u32)e[13] << 4))) |
                    (((u32)e[17] << 20) | 0x03000000u);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = ((u32)e[22] << 16) | ((u32)e[21] << 12) | (((u32)e[20] << 8) | (e[18] | ((u32)e[19] << 4))) |
                    (((u32)e[23] << 20) | 0x04000000u);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFEFFFF00u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x0F000000u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFEFFFF00u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x0F000000u;
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 5;
    f32 halfH = 0.5f * (f32)height;
    GXWGFifo._u16 = 0x101A;
    f32 halfW = 0.5f * (f32)width;
    GXWGFifo._f32 = halfW;
    GXWGFifo._f32 = (f32)(-halfH);
    GXWGFifo._f32 = 1.6777216e7f;
    GXWGFifo._f32 = 342.0f + halfW;
    GXWGFifo._f32 = 342.0f + halfH;
    GXWGFifo._f32 = 1.6777216e7f;

    s32 top;
    s32 bottom;
    if (field != 0) {
        top = 0x156;
        s32 t = 0x108;
        if (pal == 0) t = (height >> 1) + 2;
        bottom = t + 0x156;
    } else {
        s32 t = 0x102;
        if (pal == 0) t = (height >> 1) - 2;
        top = t + 0x156;
        bottom = height + 0x156;
    }
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = ((u32)(u16)top | 0x20150000u) | 0x6000u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = ((((u32)(width + 0x155)) << 12) & 0x0FFFF000u) | ((u32)(u16)(bottom - 1) | 0x21000000u);
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = ((((u32)top) << 9) & 0x01FFFC00u) | 0x59000000u | 0xABu;
    s32 reg = lbl_805FB508 | 0x42;
    lbl_805FB514 = reg;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = (u32)reg | 0x43000000u;
    lbl_805FB50C = (u8)(lbl_805FB50C & ~4);
}

// 0x8014FCB8
// Ensures the current display list has room (ends and submits it and starts the next one if not),
// then resets the GX state to defaults and writes the default Z/blend/scissor registers.
void fn_8014FCB8(void) {
    if (fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x15BA > lbl_805FB4C8) {
        if (lbl_805FB4C0 != 0) {
            RendDLEnd((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94));
            s32 rem = 0x24000 - lbl_805FB4C8;
            if (rem != 0) {
                fn_80142ED0(RendDLGetBase((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94)) + lbl_805FB4C8, rem);
            }
            if (lbl_805FB4D4 != 0) {
                lbl_805FB4DC = lbl_805FB4DC + 1;
            } else {
                RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + lbl_805FB4BC * 0x14), lbl_805FB4E0 == 0);
            }
            lbl_805FB4C0 = 0;
            lbl_805FB4D0 = NULL;
            lbl_805FB4C4 = 0;
            lbl_805FB4C8 = 0;
            lbl_805FB4CC = 0;
            if (lbl_805FB4E0 != 0) {
                lbl_805FB4E8 = lbl_805FB4E8 + 1;
            }
            lbl_805FB4BC = lbl_805FB4BC + 1;
            if (lbl_805FB4BC >= 0x11) {
                lbl_805FB4BC = 0;
            }
            if (lbl_805FB4D4 != 0 && !RendHasFreeDL() && lbl_805FB4D4 != 0) {
                RendThreadSync();
                s32 head = lbl_805FB4D8;
                s32 count = lbl_805FB4DC;
                while (count > 0) {
                    RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + head * 0x14), lbl_805FB4E0 == 0);
                    head = lbl_805FB4D8 + 1;
                    lbl_805FB4D8 = head;
                    if (head >= 0x11) {
                        lbl_805FB4D8 = 0;
                        head = 0;
                    }
                    count = lbl_805FB4DC - 1;
                    lbl_805FB4DC = count;
                }
                lbl_805FB4D4 = 0;
                lbl_805FB4D8 = 0;
                lbl_805FB4DC = 0;
            }
            RendWaitFreeDL();
            RendDLBegin((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94), (RendDLRecord*)(sRendDLRecords + lbl_805FB4BC * 0x14));
            lbl_805FB4C0 = 1;
            lbl_805FB4C4 = 0x24000;
            lbl_805FB4C8 = 0x24000;
            lbl_805FB4CC = 0;
            lbl_805FB4D0 = RendDLGetBase((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94));
            lbl_805FB4EC = lbl_805FB4EC - 1;
        }
        fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94);
    }
    RendSetDefaultState();
    lbl_805FB50C = 0;
    lbl_805FB50D = 0;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00000Fu;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0;
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = 0x103F;
    GXWGFifo._u32 = 0;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE003C00u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0;
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = 0x100E;
    GXWGFifo._u32 = 0x442;
    GXWGFifo._u8 = 0x10;
    GXWGFifo._u16 = 0;
    GXWGFifo._u16 = 0x1010;
    GXWGFifo._u32 = 0x442;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0x28380000u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xC008FFFAu;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xC108FFD0u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xFE00C000u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0;
    s32 reg = lbl_805FB514 | 0x40;
    lbl_805FB514 = reg;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = (u32)reg | 0x43000000u;
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = 0xF33F0000u;
    fn_8015B7D4();
}

// 0x80156F70
// Fills the selected 256-entry colour table (opaque black base) with a ramp on one channel:
// table 0 steps the low byte, table 1 the next byte, table 2 the third byte. Does nothing when
// `enable` is 0.
void fn_80156F70(s32 table, s32 enable) {
    if (enable == 0) return;
    if (table < 0 || table > 2) return;
    u32* p = *(u32**)(lbl_80401C30 + table * 4);
    u32 step = (table == 0) ? 1u : (table == 1) ? 0x100u : 0x10000u;
    u32 v = 0xFF000000u;
    for (s32 n = 0; n < 256; n++) {
        p[n] = v;
        v += step;
    }
}

// 0x80157324
// Converts the three 256-entry colour tables (ARGB-style words) into the uploaded palette
// blocks: swaps the red and blue bytes and forces alpha to 0xFF, then flushes each block.
void fn_80157324(void) {
    s32 sp8;
    u8** tables = (u8**)lbl_80401C30;
    u8* block = lbl_8040ACB0 + (*(u32*)lbl_805FB518 & 1) * 0x60;
    for (s32 t = 0; t < 3; t++) {
        u32* d = (u32*)fn_8015A210(block, 0, &sp8, 6);
        u32* s = (u32*)tables[t];
        for (s32 n = 0; n < 256; n++) {
            u32 x = s[n];
            d[n] = ((x << 16) & 0xFF0000u) | 0xFF000000u | (x & 0xFF00u) | ((x >> 16) & 0xFFu);
        }
        fn_8015A2E8(block, 0, 1);
        block += 0x20;
    }
}

}  // extern "C"
