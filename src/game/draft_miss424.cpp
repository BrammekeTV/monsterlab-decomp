// Missing functions batch 424: draw-list sprite/light submit (fn_801727EC) and the per-frame
// GX state refresh with zone resource preload (fn_80009BE0).
#include "types.h"
#include "revolution.h"
#include "engine/math.h"

extern "C" {
void   fn_8010AA0C(u8* res);                                     // 0x8010AA0C
void   fn_80155724(u8* m);                                       // 0x80155724
void   fn_801C849C(u8* mgr, u8* item);                           // 0x801C849C
u8*    fn_801D5850(void);                                        // 0x801D5850
void   fn_801D679C(void* unused, f32* src);                      // 0x801D679C
f64    fn_8033F20C(f64 x, f64 y);                                // 0x8033F20C (pow)
BOOL   ZoneIsLoaded(void* world, s32 zone);                      // 0x8012F408

extern f32 lbl_804010D8[12];       // 0x804010D8 (3x4 view matrix)
extern u8  lbl_804129E8[];         // 0x804129E8 (draw statistics)
extern u8  lbl_803C4298[];         // 0x803C4298
extern u8  lbl_8047C640[];         // 0x8047C640
extern u8  lbl_8047C760[];         // 0x8047C760 (effect manager)
extern u8  lbl_805FB50C;           // 0x805FB50C (GX shadow flags byte)
extern u8  gGame[];                // 0x803B53A0
extern u8  gWorldState[];          // 0x803E6D00
}
extern "C++" s32 gZoneResourceSize;   // 0x805FB94C

static inline s32 Clamp255(s32 v) {
    if (v < 0) return 0;
    if (v > 0xFF) return 0xFF;
    return v;
}

extern "C" {

// 0x801727EC
// Submits one light/sprite record to the GX XF registers (light index idx): position transformed
// by the view matrix, direction (type != 1: attenuation from the record), colour and the extra
// vector, then bumps the light counter in the draw statistics.
void fn_801727EC(void* handle, s32 idx) {
    u8* p = (u8*)handle;
    s32 base = idx << 4;
    const f32* m = lbl_804010D8;

    if (*(s16*)(p + 0x10) == 1) {
        f32 x = *(f32*)(p + 0x20);
        f32 y = *(f32*)(p + 0x24);
        f32 z = *(f32*)(p + 0x28);
        f32 px = m[9]  + ((m[6] * z) + ((m[0] * x) + (m[3] * y)));
        f32 py = m[10] + ((m[7] * z) + ((m[1] * x) + (m[4] * y)));
        f32 pz = m[11] + ((m[8] * z) + ((m[2] * x) + (m[5] * y)));
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = (u16)(base + 0x60A);
        GXWGFifo._f32 = px;
        GXWGFifo._f32 = py;
        GXWGFifo._f32 = pz;
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = (u16)(base + 0x60D);
        GXWGFifo._f32 = 0.0f;
        GXWGFifo._f32 = 0.0f;
        GXWGFifo._f32 = 1.0f;
        f32 c0 = *(f32*)(p + 0x44);
        f32 c1 = *(f32*)(p + 0x48);
        f32 c2 = *(f32*)(p + 0x4C);
        f32 c3 = *(f32*)(p + 0x50);
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = (u16)(base + 0x604);
        GXWGFifo._f32 = 1.0f;
        GXWGFifo._f32 = 0.0f;
        GXWGFifo._f32 = 0.0f;
        s32 r0 = Clamp255((s32)(255.0f * c0));
        s32 r1 = Clamp255((s32)(255.0f * c1));
        s32 r2 = Clamp255((s32)(255.0f * c2));
        s32 r3 = Clamp255((s32)(255.0f * c3));
        f32 e0 = *(f32*)(p + 0x54);
        f32 e1 = *(f32*)(p + 0x58);
        f32 e2 = *(f32*)(p + 0x5C);
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 0;
        GXWGFifo._u16 = (u16)(base + 0x603);
        GXWGFifo._u32 = (u32)(r3 | (r2 << 8) | ((r0 << 24) | (r1 << 16)));
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = (u16)(base + 0x607);
        GXWGFifo._f32 = e0;
        GXWGFifo._f32 = e1;
        GXWGFifo._f32 = e2;
    } else {
        f32 x = *(f32*)(p + 0x20);
        f32 y = *(f32*)(p + 0x24);
        f32 z = *(f32*)(p + 0x28);
        f32 d0 = *(f32*)(p + 0x38);
        f32 d1 = *(f32*)(p + 0x3C);
        f32 d2 = *(f32*)(p + 0x40);
        f32 px = m[9]  + ((m[6] * z) + ((m[0] * x) + (m[3] * y)));
        f32 py = m[10] + ((m[7] * z) + ((m[1] * x) + (m[4] * y)));
        f32 pz = m[11] + ((m[8] * z) + ((m[2] * x) + (m[5] * y)));
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = (u16)(base + 0x60A);
        GXWGFifo._f32 = px;
        GXWGFifo._f32 = py;
        GXWGFifo._f32 = pz;
        f32 spread = *(f32*)(p + 0x1C);
        f32 nx = -d0;
        f32 ny = -d1;
        f32 nz = -d2;
        // direction rotated by the view matrix (no translation)
        f32 dx = (m[6] * nz) + ((m[0] * nx) + (m[3] * ny));
        f32 dy = (m[7] * nz) + ((m[1] * nx) + (m[4] * ny));
        f32 dz = (m[8] * nz) + ((m[2] * nx) + (m[5] * ny));
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = (u16)(base + 0x60D);
        GXWGFifo._f32 = dx;
        GXWGFifo._f32 = dy;
        GXWGFifo._f32 = dz;
        f32 a0, a1, a2;
        if (spread < 2.0f) {
            a1 = 4.0f;
            a2 = 0.0f;
            a0 = -4.0f * ((5.0f * *(f32*)(p + 0x18)) / 6.0f);
        } else {
            f32 s = 1.0f;
            if (!(spread < 1.0f)) {
                s = 10.0f;
                if (!(spread > 10.0f)) s = spread;
            }
            a1 = 0.0f;
            a2 = 4.0f;
            a0 = -4.0f * (f32)fn_8033F20C(0.0625, (f64)(1.0f / s));
        }
        f32 c0 = *(f32*)(p + 0x44);
        f32 c1 = *(f32*)(p + 0x48);
        f32 c2 = *(f32*)(p + 0x4C);
        f32 c3 = *(f32*)(p + 0x50);
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = (u16)(base + 0x604);
        GXWGFifo._f32 = a0;
        GXWGFifo._f32 = a1;
        GXWGFifo._f32 = a2;
        s32 r0 = Clamp255((s32)(255.0f * c0));
        s32 r1 = Clamp255((s32)(255.0f * c1));
        s32 r2 = Clamp255((s32)(255.0f * c2));
        s32 r3 = Clamp255((s32)(255.0f * c3));
        f32 e0 = *(f32*)(p + 0x54);
        f32 e1 = *(f32*)(p + 0x58);
        f32 e2 = *(f32*)(p + 0x5C);
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 0;
        GXWGFifo._u16 = (u16)(base + 0x603);
        GXWGFifo._u32 = (u32)(r3 | (r2 << 8) | ((r0 << 24) | (r1 << 16)));
        GXWGFifo._u8  = 0x10;
        GXWGFifo._u16 = 2;
        GXWGFifo._u16 = (u16)(base + 0x607);
        GXWGFifo._f32 = e0;
        GXWGFifo._f32 = e1;
        GXWGFifo._f32 = e2;
    }
    *(s32*)(lbl_804129E8 + 0x3C) += 1;
}

// 0x80009BE0
// Per-frame refresh: resets the camera matrix, queues the GX line/fog state when the shadow flag
// asks for it, preloads every loaded zone's resources, then releases the held effect.
// The original is an inlined helper called with idx = -1, so its other branches (effect
// spawn/update, plus a GX block that the entry jump skips) are statically dead and omitted.
void fn_80009BE0(void) {
    f32 mtx[12] = { 1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 0.0f };
    fn_801D679C(lbl_8047C640, mtx);
    Vec3 axis = QuatGetAxisY((const Quat*)(*(u8**)(*(u8**)(lbl_803C4298 + 0x4) + 0x4) + 0xC));
    (void)axis;
    fn_80155724(fn_801D5850());

    u8 flags = lbl_805FB50C;
    if (flags & 2) {
        lbl_805FB50C = flags & ~2;
        if (flags & 1) {
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0xFE00000Fu;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8  = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x103F;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0xFE003C00u;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8  = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x1010;
            GXWGFifo._u32 = 0x441;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0x28380000u;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0xC008F2A4u;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0xC108E6F0u;
        } else {
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0xFE00000Fu;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8  = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x103F;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0xFE003C00u;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0;
            GXWGFifo._u8  = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x100E;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8  = 0x10;
            GXWGFifo._u16 = 0;
            GXWGFifo._u16 = 0x1010;
            GXWGFifo._u32 = 0x442;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0x28380000u;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0xC008FFFAu;
            GXWGFifo._u8  = 0x61;
            GXWGFifo._u32 = 0xC108FFD0u;
        }
    }

    // Preload resources of every loaded zone.
    for (s32 zone = 0; zone <= *(s32*)(*(u8**)gWorldState + 0x4); zone++) {
        if (ZoneIsLoaded(gWorldState, zone)) {
            u8* res = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
            s32 off = 0;
            for (s32 i = 0; i < (s32)*(u8*)(res + 0xA4); i++) {
                fn_8010AA0C(*(u8**)(res + 0xA0) + off);
                off += 0x4C;
            }
        }
    }

    u8* held = *(u8**)(gGame + 0x138);
    if (held != NULL) fn_801C849C(lbl_8047C760, held);
}

}
