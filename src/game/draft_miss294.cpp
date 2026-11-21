// Missing functions batch 294: world draw pass (fn_8000A6AC), boot-time world/settings setup
// (fn_8000E488), brain-lab chunk throw target (fn_8007EF04).
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "revolution.h"
#include "engine/math.h"
#include "engine/view.h"
#include "game/game.h"

extern "C" {

void CameraSetMatrix(void* camera, const f32 mtx[3][4]);   // 0x801D6814
u8*  GetGlobalSettings(void);                              // 0x801D2BF8
void* MemAllocDefault(u32 size);                           // 0x801D4650
void RendClear(int flags, int a, int b, f32 depth);        // 0x801534F0
s32  ScreenSetsDraw(void* sets, int pass);                 // 0x8018CD70
f32  TimerReset(u8* timer);                                // 0x80142744
f32  fn_8033F1FC(f32 a, f32 b);                            // 0x8033F1FC
void fn_80009BE0(void);                                    // 0x80009BE0
void fn_80081F04(u8* obj);                                 // 0x80081F04
void fn_80133E60(u8* world);                               // 0x80133E60
void fn_8014DE68(void);                                    // 0x8014DE68
void fn_80150148(void);                                    // 0x80150148
void fn_801514D4(void);                              // 0x801514D4
void fn_801514F0(void);                                    // 0x801514F0
void fn_80154294(void);                                    // 0x80154294
void fn_801542DC(void);                                    // 0x801542DC
void fn_80154318(void);                                    // 0x80154318
s32  fn_8015BA30(void* p);                                 // 0x8015BA30
u8*  fn_80142500(void* p);                                 // 0x80142500
void fn_801214DC(void* p);                                 // 0x801214DC
void fn_801214E0(void);                                 // 0x801214E0
void fn_80161A28(u8* arg0, f32 dt);                                // 0x80161A28
void fn_801620BC(void* fx);                                // 0x801620BC
void fn_8016E304(void* cam);                               // 0x8016E304
void fn_8016FB20(void* view);                              // 0x8016FB20
void fn_80170044(void* view);                              // 0x80170044
void fn_8018A3E4(void* blk);                               // 0x8018A3E4
void fn_8018C8EC(void* blk);                               // 0x8018C8EC
u8*  fn_801B161C(u8* p, u32 key);                          // 0x801B161C
void* fn_801B18E4(u8* list);                               // 0x801B18E4
void fn_801B42B4(void* p);                                 // 0x801B42B4
void fn_801B432C(void* blk);                               // 0x801B432C
void fn_801BB3CC(void* blk);                               // 0x801BB3CC
void fn_801BC868(void* blk);                               // 0x801BC868
void fn_801BE3A4(u8* p);                                   // 0x801BE3A4
void fn_801C37EC(void* blk);                               // 0x801C37EC
void fn_801C3A64(u8* p);                                   // 0x801C3A64
void fn_801CAD70(void* mgr, void* view);                   // 0x801CAD70
void fn_801CB414(void* mgr, int a);                        // 0x801CB414
void fn_801D2C00(void* blk);  // 0x801D2C00
u8*  fn_801D66B4(void);                                    // 0x801D66B4
void fn_801D66BC(void* blk);                               // 0x801D66BC
void fn_801D66C4(f32 a, f32 b);                            // 0x801D66C4
void fn_801D6780(void);                                    // 0x801D6780
void fn_801D6FD0(void);                               // 0x801D6FD0
void fn_801D7350(void);                               // 0x801D7350
void fn_801D7728(void* cam);                               // 0x801D7728
void fn_801D7924(void* cam);                               // 0x801D7924
void fn_801D81FC(void* cam);                               // 0x801D81FC
void fn_801D88B0(void* cam);                               // 0x801D88B0
void fn_801E27C8(void* blk);                               // 0x801E27C8
void fn_801E28E0(void* p);                                 // 0x801E28E0
void fn_801E47D8(void* pool);                              // 0x801E47D8
void fn_801E5478(void* pool, void* fn);                    // 0x801E5478
void fn_801E5480(void* pool, void* fn);                    // 0x801E5480
void fn_801E54E4(void* pool, const char* name, u8* buf, u32 a, u32 size);  // 0x801E54E4
void fn_801EFC74(void* blk);                               // 0x801EFC74
void fn_801ABD50(void* blk);                               // 0x801ABD50
void fn_80204478(void);                                    // 0x80204478
void fn_80208CF0(void* blk);                               // 0x80208CF0
void* memset(void* p, int c, unsigned long n);

extern u8  gIdPool[];                  // 0x8048A8F0
extern u8  gWorldState[];              // 0x803E6D00
extern f32 lbl_803464D8[];             // 0x803464D8
extern f32 lbl_803464E4[];             // 0x803464E4
extern u8  lbl_803C2A00[];             // 0x803C2A00 (+0x20: screen-set owner)
extern u8  lbl_803C4298[];             // 0x803C4298 (+4: object with vtable)
extern s32 lbl_803C44A0[];             // 0x803C44A0 (default settings, 13 words)
extern u8  lbl_8040E5F0[];             // 0x8040E5F0 (frame effects object)
extern u8  lbl_8047C640[];             // 0x8047C640 (camera)
extern u8  lbl_8047C760[];             // 0x8047C760 (effect manager)
extern u8  lbl_8047C790[];             // 0x8047C790
extern u8  lbl_8047C7AC[];             // 0x8047C7AC
extern u8  lbl_8047C7D8[];             // 0x8047C7D8
extern u8  lbl_8048A8B4[];             // 0x8048A8B4 (SIR loader object)
extern const char lbl_8035374C[];      // 0x8035374C ("catalog.sir")
extern u8  lbl_805F81A4;               // 0x805F81A4
extern u8  lbl_805F9211;               // 0x805F9211
extern u8  lbl_805F9212;               // 0x805F9212 (copy filter value)
extern u32 lbl_805F9614;               // 0x805F9614 (frame-setup done flag)
extern f32 lbl_805FAE6C;               // 0x805FAE6C
extern u8  lbl_805FAE70;               // 0x805FAE70
extern u8  lbl_805FAE71;               // 0x805FAE71
extern u8  lbl_805FAE78[];             // 0x805FAE78 (timer)
extern u8  lbl_805FB2F3;               // 0x805FB2F3
extern s32 lbl_805FB4BC;               // 0x805FB4BC (current display list)
extern s32 lbl_805FB4C0;               // 0x805FB4C0
extern s32 lbl_805FB4C8;               // 0x805FB4C8
extern u8  lbl_805FB50C;               // 0x805FB50C (GX shadow flags byte)
extern u8  lbl_805FB50E;               // 0x805FB50E
extern s32 lbl_805FB514;               // 0x805FB514 (GX shadow word)
extern s32 lbl_805FB790;               // 0x805FB790
extern u8  lbl_805FB8A8[];             // 0x805FB8A8
}

extern "C++" u8   sWorldReady;         // 0x805FB2F0
extern "C++" View* gMainView;          // 0x805FB75C
extern "C++" void* lbl_805FB680;       // 0x805FB680 (active camera)
extern "C++" u8   sRendDLs[];          // 0x80401108 (display-list records, 0x94 bytes each)
extern "C++" s32  fn_80014328(void);   // 0x80014328
extern "C++" void AudioUpdateListener(void);    // 0x8001011C
extern "C++" void SysRequestReset(void);        // 0x80010FE8
extern "C++" void DiscErrorHandlerExt(void);    // 0x80010FF0
struct WorldParams;
extern "C++" void WorldDataCreate(WorldParams* params);  // 0x801F0704
extern "C++" void fn_80154358(void);            // 0x80154358
extern "C++" LoadedBuffer gBuffer803C73EC, gBuffer803C7404, gBuffer803C741C, gBuffer803C7434,
                          gBuffer803C744C, gBuffer803C7464, gBuffer803C747C;

static const u32 kHashBucket = 0x17C7F120u;   // "Bucket"

static const f32 kHalfPi = 1.5707964f;
static const f32 kTwoOverPi = 0.63661975f;

// sqrt that returns 0 for non-positive input (the original guards frsqrte the same way).
static inline f32 SqrtGuard(f32 v) {
    return v <= 0.0f ? 0.0f : sqrtf(v);
}

// Display-list overflow check used between draw stages: flushes when the current list is over budget.
static inline void RendBudgetCheck(void) {
    if (lbl_805FB4C0 != 0 && fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x10C0 > lbl_805FB4C8) {
        fn_8014DE68();
    }
}

typedef void (*CamMatrixFn)(void*, f32*);

extern "C" {

// 0x8000A6AC
// World draw pass. With the world ready: sets up the main view and camera, runs the effect and
// screen-set passes, then renders the mirrored (negated first row) view into the camera copy at
// lbl_803C2A00+0x1680 and makes it the active camera. Without a world: draws an empty frame
// with an identity camera.
void fn_8000A6AC(void) {
    u8* sets = lbl_803C2A00 + 0x20;
    u8* view;
    if (sWorldReady != 0 && lbl_805FB2F3 != 0) {
        if (lbl_805F9614 != 1) {
            f32 m[3][4];
            lbl_805F9614 = 1;
            fn_801D6780();
            view = (u8*)gMainView;
            *(s32*)(view + 0x188) = 1;
            fn_801514D4();
            u8* cam = *(u8**)(lbl_803C4298 + 4);
            (*(CamMatrixFn*)(*(u8**)cam + 0x38))(cam, &m[0][0]);
            CameraSetMatrix(lbl_8047C640, m);
            fn_80154294();
            fn_80154318();
            lbl_805FB50C |= 8;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0xFE00C000u;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = (u32)lbl_805F9212 << 14;
        }
        fn_80133E60(gWorldState);
        RendBudgetCheck();
        fn_80009BE0();
        fn_801214DC(sets + 0x10);
        u8* v = fn_801D66B4();
        fn_801D66C4(*(f32*)(v + 0x8) + lbl_805FAE6C, *(f32*)(v + 0xC) - 2.0f * lbl_805FAE6C);
        fn_80154358();
        fn_801E28E0(lbl_805FB8A8);
        fn_801D6780();
        fn_80154318();
        fn_801CB414(lbl_8047C760, 0);
        fn_801BE3A4(lbl_8047C790);   // asm also leaves the frame delta (gWorldState+0xC) in f1; the callee ignores it
        fn_801C3A64(lbl_8047C7AC);
        lbl_805FB790 = 0;
        fn_80170044((void*)gMainView);
        RendBudgetCheck();
        ScreenSetsDraw(*(void**)(sets + 0xDB0), 0);
        if ((s8)lbl_805FAE71 == 0) {
            fn_80142500(lbl_805FAE78);
            lbl_805FAE71 = 1;
        }
        TimerReset(lbl_805FAE78);
        fn_80161A28(lbl_8040E5F0, 0.0f);  // dt is a leftover f1 in the asm
        fn_801620BC(lbl_8040E5F0);
        if (lbl_805FAE70 == 0) {
            RendClear(4, 0, 0, 1.0f);
        }
        if (lbl_805F81A4 != 0 && !(lbl_805FB50C & 4)) {
            u8 flags = lbl_805FB50C;
            s32 w = lbl_805FB514;
            lbl_805FB50C = flags | 4;
            w &= ~0x40;
            lbl_805FB514 = w;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = (u32)w | 0x43000000u;
            u32 b = lbl_805FB50E;
            b = (b & ~0x00FF0000u) | ((u32)lbl_805F9211 << 16);
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = b | 0xF3380000u;
        }
        ScreenSetsDraw(*(void**)(sets + 0xDB0), 1);
        RendBudgetCheck();
        fn_8016FB20((void*)gMainView);
        fn_80154294();
        fn_801D7728(lbl_8047C640);
        fn_801D7924(lbl_8047C640);
        fn_801D81FC(lbl_8047C640);
        fn_801D88B0(lbl_8047C640);
        fn_801D6FD0();
        fn_801D7350();
        fn_801542DC();
        fn_801214E0();
        fn_801B42B4(lbl_8047C7D8);
        fn_80170044((void*)gMainView);
        RendBudgetCheck();

        // Second view setup, then render again from the mirrored camera.
        f32 m2[3][4];
        lbl_805F9614 = 1;
        fn_801D6780();
        view = (u8*)gMainView;
        *(s32*)(view + 0x188) = 1;
        u8* cam = *(u8**)(lbl_803C4298 + 4);
        (*(CamMatrixFn*)(*(u8**)cam + 0x38))(cam, &m2[0][0]);
        CameraSetMatrix(lbl_8047C640, m2);
        fn_80154294();
        fn_80154318();
        lbl_805FB50C |= 8;
        GXWGFifo._u8  = 0x61;
        GXWGFifo._u32 = 0xFE00C000u;
        GXWGFifo._u8  = 0x61;
        GXWGFifo._u32 = (u32)lbl_805F9212 << 14;
        fn_8016FB20((void*)gMainView);

        // Mirrored view record (0x1A4 bytes) copied from the main view, first three floats negated.
        u8* mir = lbl_803C2A00 + 0x1680;
        view = (u8*)gMainView;
        for (u32 off = 0; off < 0x1A4; off += 4) {
            *(u32*)(mir + off) = *(u32*)(view + off);
        }
        view = (u8*)gMainView;
        u8* mir2 = lbl_803C2A00 + 0x1828;
        for (u32 off = 0; off < 0x30; off += 4) {
            *(u32*)(mir2 + off) = *(u32*)(view + off);
        }
        f32 nx = -*(f32*)(view + 0x0);
        f32 ny = -*(f32*)(view + 0x4);
        f32 nz = -*(f32*)(view + 0x8);
        *(f32*)(mir2 + 0x0) = nx;
        *(f32*)(mir2 + 0x4) = ny;
        *(f32*)(mir2 + 0x8) = nz;
        *(f32*)(mir + 0x0) = nx;
        *(f32*)(mir + 0x4) = ny;
        *(f32*)(mir + 0x8) = nz;
        fn_8016E304(mir);
        lbl_805FB680 = mir;
        fn_801CB414(lbl_8047C760, 1);
        ScreenSetsDraw(*(void**)(sets + 0xDB0), 2);
        *(s32*)((u8*)gMainView + 0x188) = 1;
        fn_801CAD70(lbl_8047C760, (void*)gMainView);
        fn_80170044((void*)gMainView);
        RendBudgetCheck();
    } else {
        fn_80150148();
        fn_8016FB20((void*)gMainView);
        fn_801D66C4(0.5f, 1000.0f);
        *(s32*)((u8*)gMainView + 0x188) = 0;
        f32 id[3][4];
        id[0][0] = 1.0f; id[0][1] = 0.0f; id[0][2] = 0.0f; id[0][3] = 0.0f;
        id[1][0] = 0.0f; id[1][1] = 1.0f; id[1][2] = 0.0f; id[1][3] = 0.0f;
        id[2][0] = 0.0f; id[2][1] = 0.0f; id[2][2] = 1.0f; id[2][3] = 0.0f;
        CameraSetMatrix(lbl_8047C640, id);
        fn_801514F0();
        lbl_805FB50C &= ~8;
        GXWGFifo._u8  = 0x61;
        GXWGFifo._u32 = 0xFE00C000u;
        GXWGFifo._u8  = 0x61;
        GXWGFifo._u32 = 0;
        fn_801542DC();
        fn_80154318();
        GXWGFifo._u8  = 0x61;
        GXWGFifo._u32 = 0xFE000FE0u;
        GXWGFifo._u8  = 0x61;
        GXWGFifo._u32 = 0x410004A0u;
        fn_801B42B4(lbl_8047C7D8);
        fn_80170044((void*)gMainView);
    }
}

// Sum of the per-category zone counts in the global settings (13 fields), the capacity base
// for every zone table.
static inline s32 ZoneTotal(void) {
    s32 n = *(s32*)(GetGlobalSettings() + 0x80);
    n = *(s32*)(GetGlobalSettings() + 0x68) + n;
    n += *(s32*)(GetGlobalSettings() + 0x84);
    n += *(s32*)(GetGlobalSettings() + 0x88);
    n += *(s32*)(GetGlobalSettings() + 0x8C);
    n += *(s32*)(GetGlobalSettings() + 0x90);
    n += *(s32*)(GetGlobalSettings() + 0x94);
    n += *(s32*)(GetGlobalSettings() + 0x98);
    n += *(s32*)(GetGlobalSettings() + 0x6C);
    n += *(s32*)(GetGlobalSettings() + 0x70);
    n += *(s32*)(GetGlobalSettings() + 0x74);
    n += *(s32*)(GetGlobalSettings() + 0x78);
    n += *(s32*)(GetGlobalSettings() + 0x7C);
    return n;
}

// 0x8000E488
// Boot-time engine setup: builds the global settings block (default record plus fixed values),
// registers a series of small configuration blocks with their setters, initializes the SIR
// loader ("catalog.sir" at MEM2 0x90400000) and allocates the per-zone tables sized from the
// settings (the original also had an ObjAlloc branch for buffer kind 2, never taken here).
void fn_8000E488(void) {
    u8* blk = (u8*)MemAllocDefault(0x9C);
    for (u32 i = 0; i < 13; i++) {
        *(s32*)(blk + i * 4) = lbl_803C44A0[i];
    }
    *(s32*)(blk + 0x34) = 1;
    *(s32*)(blk + 0x38) = 2;
    *(s32*)(blk + 0x3C) = 2;
    *(s32*)(blk + 0x40) = 2;
    *(s32*)(blk + 0x44) = 2;
    *(s32*)(blk + 0x48) = 2;
    *(s32*)(blk + 0x4C) = 1;
    *(s32*)(blk + 0x50) = 1;
    *(s32*)(blk + 0x54) = 1;
    *(s32*)(blk + 0x58) = 0;
    *(s32*)(blk + 0x5C) = 0;
    *(s32*)(blk + 0x60) = 0;
    *(s32*)(blk + 0x64) = 0;
    *(s32*)(blk + 0x68) = 0x14;
    *(s32*)(blk + 0x6C) = 0x48;
    *(s32*)(blk + 0x70) = 0x3C;
    *(s32*)(blk + 0x74) = 0x78;
    *(s32*)(blk + 0x78) = 0x78;
    *(s32*)(blk + 0x7C) = 0x3C;
    *(s32*)(blk + 0x80) = 0x13;
    *(s32*)(blk + 0x84) = 9;
    *(s32*)(blk + 0x88) = 0xC;
    *(s32*)(blk + 0x8C) = 0;
    *(s32*)(blk + 0x90) = 0;
    *(s32*)(blk + 0x94) = 0;
    *(s32*)(blk + 0x98) = 0;
    fn_801D2C00(blk);

    blk = (u8*)MemAllocDefault(0x14);
    *(f32*)(blk + 0x8) = 0.4f;
    *(f32*)(blk + 0xC) = 100.0f;
    *(f32*)(blk + 0x0) = 0.4f;
    *(f32*)(blk + 0x4) = 100.0f;
    *(f32*)(blk + 0x10) = 120000.0f;
    fn_801D66BC(blk);

    blk = (u8*)MemAllocDefault(4);
    *(s32*)blk = 4;
    fn_801E27C8(blk);

    blk = (u8*)MemAllocDefault(2);
    *(u8*)blk = 1;
    fn_8018C8EC(blk);

    blk = (u8*)MemAllocDefault(1);
    *(u8*)blk = 0;
    fn_801EFC74(blk);

    blk = (u8*)MemAllocDefault(1);
    *(u8*)blk = 0;
    fn_801BC868(blk);

    blk = (u8*)MemAllocDefault(1);
    *(u8*)blk = 0;
    fn_801B432C(blk);

    blk = (u8*)MemAllocDefault(1);
    *(u8*)blk = 0;
    fn_801BB3CC(blk);

    blk = (u8*)MemAllocDefault(1);
    *(u8*)blk = 0;
    fn_8018A3E4(blk);

    blk = (u8*)MemAllocDefault(0xC);
    *(s32*)(blk + 0x0) = 1;
    *(void**)(blk + 0x8) = (void*)fn_80014328;
    *(u8*)(blk + 0x4) = 0;
    fn_801ABD50(blk);

    blk = (u8*)MemAllocDefault(0xC);
    *(s32*)(blk + 0x4) = 0xC8;
    *(s32*)(blk + 0x0) = 0x1F4;
    *(void**)(blk + 0x8) = gWorldState;
    WorldDataCreate((WorldParams*)blk);

    blk = (u8*)MemAllocDefault(0xC);
    *(s32*)(blk + 0x0) = 1;
    *(f32*)(blk + 0x4) = 0.3f;
    *(void**)(blk + 0x8) = (void*)AudioUpdateListener;
    fn_80208CF0(blk);

    blk = (u8*)MemAllocDefault(4);
    *(s32*)blk = 1;
    fn_801C37EC(blk);

    fn_801E47D8(lbl_8048A8B4);
    fn_801E5478(lbl_8048A8B4, (void*)SysRequestReset);
    fn_801E5480(lbl_8048A8B4, (void*)DiscErrorHandlerExt);
    fn_801E54E4(gIdPool, lbl_8035374C, (u8*)(uintptr_t)0x90400000u, *(u32*)GetGlobalSettings(), 0x40000);
    fn_80204478();   // the original also passes lbl_805FB7E8 in r3, which the callee ignores

    // Per-zone tables, each {data, count, kind} with count = zones + 1 and kind 1 (owned).
    s32 n;
    n = ZoneTotal() + 1;
    gBuffer803C73EC.kind = 1;
    gBuffer803C73EC.size = (u32)n;
    gBuffer803C73EC.data = MemAllocDefault((u32)n * 4);

    n = ZoneTotal() + 1;
    gBuffer803C741C.kind = 1;
    gBuffer803C741C.size = (u32)n;
    gBuffer803C741C.data = MemAllocDefault((u32)n * 4);

    n = ZoneTotal() + 1;
    gBuffer803C7434.kind = 1;
    gBuffer803C7434.size = (u32)n;
    gBuffer803C7434.data = MemAllocDefault((u32)n * 4);

    n = ZoneTotal() + 1;
    gBuffer803C744C.kind = 1;
    gBuffer803C744C.size = (u32)n;
    gBuffer803C744C.data = MemAllocDefault((u32)n);

    n = ZoneTotal() + 1;
    gBuffer803C7404.kind = 1;
    gBuffer803C7404.size = (u32)n;
    gBuffer803C7404.data = MemAllocDefault((u32)n << 5);
    memset(gBuffer803C7404.data, 0, gBuffer803C7404.size << 5);

    n = ZoneTotal() + 1;
    gBuffer803C7464.kind = 1;
    gBuffer803C7464.size = (u32)n;
    gBuffer803C7464.data = MemAllocDefault((u32)n * 4);
    memset(gBuffer803C7464.data, 0, gBuffer803C7464.size * 4);

    n = ZoneTotal() + 1;
    gBuffer803C747C.kind = 1;
    gBuffer803C747C.size = (u32)n;
    gBuffer803C747C.data = MemAllocDefault((u32)n * 4);
    memset(gBuffer803C747C.data, 0, gBuffer803C747C.size * 4);
}

// Largest root of |P + D*t - C|^2 = r2 in the xy plane (P = (px,py), D = (dx,dy)); on success
// stores the hit point P + D*t in *hx, *hy and returns 1, returns 0 if the ray misses.
static int RayCircleHit(f32 px, f32 py, f32 dx, f32 dy, f32 cx, f32 cy, f32 r2, f32* hx, f32* hy) {
    f32 a = dx * dx + dy * dy;
    f32 b = 2.0f * (dx * (px - cx) + dy * (py - cy));
    f32 c = (cy * cy + (cx * cx + (px * px + py * py))) - (r2 + 2.0f * (px * cx + py * cy));
    f32 disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) {
        return 0;
    }
    f32 root = SqrtGuard(disc);
    f32 t0 = (-b + root) / (2.0f * a);
    f32 t1 = (-b - root) / (2.0f * a);
    f32 t = t0 > t1 ? t0 : t1;
    *hx = px + dx * t;
    *hy = py + dy * t;
    return 1;
}

// 0x8007EF04
// Brain lab: picks the landing point for a tossed chunk and starts the toss. The aim direction
// comes from the lab's facing plus `t`; the landing spot is where that ray leaves the circle
// around the lab centre (radius from the tables times the lab scale), then it is pulled onto the
// "Bucket" shape, or the other catch shapes, when the throw would land within their reach.
// Finishes by calling fn_80081F04 (toss setup) and clearing the lab's pending chunk index.
void fn_8007EF04(u8* mgr, u8* chunk, s32 idx, f32 t) {
    f32 flightTime = lbl_803464D8[idx - 1];
    *(f32*)(chunk + 0x274) = 0.0f;
    *(f32*)(chunk + 0x278) = flightTime;
    f32 radius = lbl_803464E4[idx - 1] * *(f32*)(mgr + 0xB4);

    // Start position (the chunk's current position) is also the ray origin.
    u8* pos = *(u8**)(chunk + 0x4);
    f32 px = *(f32*)(pos + 0x0);
    f32 py = *(f32*)(pos + 0x4);
    f32 pz = *(f32*)(pos + 0x8);
    *(f32*)(chunk + 0x280) = px;
    *(f32*)(chunk + 0x284) = py;
    *(f32*)(chunk + 0x288) = pz;

    // Ray direction: lab facing (negated Y axis of its rotation) turned by t quarter-turns.
    u8* labObj = *(u8**)(*(u8**)(mgr + 0x1F4) + 0x4);
    Vec3 axis = QuatGetAxisY((const Quat*)(labObj + 0xC));
    f32 ang = fn_8033F1FC(-axis.x, -axis.y);
    f32 q = kTwoOverPi * ang + kTwoOverPi * t;
    f32 dx = sinf(kHalfPi * q);
    f32 dy = cosf(kHalfPi * q);

    // Landing spot on the circle around the lab centre. If the ray misses, the original reads
    // uninitialized stack here; start from zero.
    u8* center = *(u8**)(*(u8**)(mgr + 0x8C) + 0x5C);
    f32 cx = *(f32*)(center + 0x8);
    f32 cy = *(f32*)(center + 0xC);
    f32 hx = 0.0f;
    f32 hy = 0.0f;
    f32 hz;
    RayCircleHit(px, py, dx, dy, cx, cy, radius * radius, &hx, &hy);
    hz = 0.0f;

    // The "Bucket" shape in the lab's shape list (NULL list if empty), and the chunk's own shape.
    u8* shapes = *(u8**)(*(u8**)(mgr + 0x29C) + 0x8C);
    u8* list = (*(s32*)(shapes + 0x10) == 0) ? NULL : **(u8***)(shapes + 0xC);
    u8* bucket = fn_801B161C(list, kHashBucket);
    u8* own = (u8*)fn_801B18E4(*(u8**)(chunk + 0x8C));
    u8* ctl = *(u8**)(mgr + 0x29C);
    s32 caught = 0;

    if (*(u8*)(ctl + 0x270) != 0 && *(s32*)(chunk + 0x264) == 1) {
        // Bucket is active and the chunk is in the aiming state: caught if the landing spot is
        // within the combined radii and there is time left.
        f32 ddx = *(f32*)(bucket + 0x14) - hx;
        f32 ddy = *(f32*)(bucket + 0x18) - hy;
        f32 dist = SqrtGuard(ddx * ddx + ddy * ddy);
        if (dist < *(f32*)(bucket + 0x3C) + *(f32*)(own + 0x3C)
            && flightTime < *(f32*)(ctl + 0x26C) - *(f32*)(ctl + 0x268)) {
            caught = 1;
            hz = *(f32*)(bucket + 0x1C);
        }
    } else if (*(u8*)(ctl + 0x284) != 0 && flightTime < *(f32*)(ctl + 0x26C) - *(f32*)(ctl + 0x268)) {
        // Secondary catch: ray against a 0.5-radius circle around the bucket; if it hits, shrink the
        // landing circle by the distance from the lab object (ctl+4) to the bucket and aim again.
        f32 bx = *(f32*)(bucket + 0x14);
        f32 by = *(f32*)(bucket + 0x18);
        f32 ux, uy;
        if (RayCircleHit(px, py, dx, dy, bx, by, 0.25f, &ux, &uy)) {
            u8* o = *(u8**)(ctl + 0x4);
            f32 ex = *(f32*)(o + 0x0) - bx;
            f32 ey = *(f32*)(o + 0x4) - by;
            f32 ez = *(f32*)(o + 0x8) - 0.0f;
            f32 r = SqrtGuard(ez * ez + (ex * ex + ey * ey));
            f32 r2 = radius - r;
            f32 h2x = 0.0f;   // uninitialized in the original when the ray misses
            f32 h2y = 0.0f;
            RayCircleHit(px, py, dx, dy, cx, cy, r2 * r2, &h2x, &h2y);
            f32 fx = *(f32*)(bucket + 0x14) - h2x;
            f32 fy = *(f32*)(bucket + 0x18) - h2y;
            f32 dist = SqrtGuard(fx * fx + fy * fy);
            if (dist < *(f32*)(bucket + 0x3C) + *(f32*)(own + 0x3C)) {
                caught = 1;
                hx = h2x;
                hy = h2y;
                hz = *(f32*)(bucket + 0x1C);
            }
        }
    } else if (*(s32*)(chunk + 0x264) != 1) {
        hz = *(f32*)(bucket + 0x1C);
    }

    // Second catch object (mgr+0x2A0): only when the bucket did not catch it.
    u8* ctl2 = *(u8**)(mgr + 0x2A0);
    if (*(u8*)(ctl2 + 0x270) != 0 && caught == 0) {
        u8* o = (u8*)fn_801B18E4(*(u8**)(ctl2 + 0x8C));
        f32 ddx = *(f32*)(o + 0x14) - hx;
        f32 ddy = *(f32*)(o + 0x18) - hy;
        f32 dist = SqrtGuard(ddx * ddx + ddy * ddy);
        if (dist < *(f32*)(o + 0x3C) + *(f32*)(own + 0x3C)) {
            hz = *(f32*)(o + 0x1C);
        }
    }

    *(f32*)(chunk + 0x28C) = hx;
    *(f32*)(chunk + 0x290) = hy;
    *(f32*)(chunk + 0x294) = hz;
    fn_80081F04(chunk);
    *(s32*)(*(u8**)(mgr + 0x1F4) + 0x268) = -1;
}

}  // extern "C"
