// Camera object: wanted position and direction, blended from the follow state and the loaded
// zones' camera sources.
// Original: 0x800DC070 (auto_03_800DB9A4_text.s)
#include "types.h"
#include "engine/math.h"

extern "C" {
f32  fn_800DE2AC(u8* zone);        // 0x800DE2AC
BOOL ZoneIsLoaded(void* world, s32 zone);      // 0x8012F408
extern u8 gWorldState[];           // 0x803E6D00 (first word: world block pointer)
}
extern "C++" s32 gZoneResourceSize;            // 0x805FB94C

// The sources share four vtable slots: 0xC fills a position and a direction and returns
// non-zero when it applies, 0x10 resets, 0x14 gives the weight and 0x18 the blend value.
typedef s32  (*SrcGetFn)(void* src, f32* pos, f32* dir, f32 dt);
typedef void (*SrcResetFn)(void* src, s32 arg);
typedef f32  (*SrcValueFn)(void* src);
#define SRC_VT(src, off, T) (*(T*)(*(u8**)(src) + (off)))

// 0x800DC070
// Called from the camera update (fn_800DC6C4). After a reset (byte at 0x18) the camera first
// takes the position of its zone block (0x2C0) and resets the follow state (+0x68). Entry 0 is
// the follow state; every source object of every loaded zone (zone resource +0xA8, count +0xAC)
// that applies adds an entry, up to ten, and sets the byte at 0x19. The follow state's weight is
// 1 minus the largest other weight (0..1). `pos` and `dir` receive the weighted sums (they are
// added to: the caller clears them), `pos` divided by the weight total and `dir` normalised;
// `blend` gets the mean of the blend values.
//
// The original calls slot 0xC with entry 10 (one past its arrays) when ten entries are in use
// and then leaves that zone's list; the buffer below has the original's layout for that call.
extern "C" void fn_800DC070(u8* cam, f32* pos, f32* dir, f32* blend, f32 dt) {
    // The original keeps ten directions at 0x80 and ten positions right after them (0xF8), so
    // the call with entry 10 writes its direction over the position of entry 0.
    Vec3 buf[21];
    Vec3* srcDir = buf;
    Vec3* srcPos = buf + 10;
    f32  weight[10];
    f32  value[10];

    *(u8*)(cam + 0x19) = 0;
    if (*(u8*)(cam + 0x18) != 0) {
        u8* zone = *(u8**)(cam + 0x2C0);
        const f32* v = *(const f32**)(zone + 0x4);
        f32 x = v[0], y = v[1], z = v[2];
        *(f32*)(cam + 0x44) = x;
        *(f32*)(cam + 0x48) = y;
        *(f32*)(cam + 0x4C) = z;
        *(f32*)(cam + 0x50) = x;
        *(f32*)(cam + 0x54) = y;
        *(f32*)(cam + 0x58) = z;
        f32 ground = fn_800DE2AC(zone);
        *(f32*)(cam + 0x5C) = ground;
        *(f32*)(cam + 0x60) = ground;
        SRC_VT(cam + 0x68, 0x10, SrcResetFn)(cam + 0x68, 0);
        *(u8*)(cam + 0x18) = 0;
    }
    SRC_VT(cam + 0x68, 0xC, SrcGetFn)(cam + 0x68, &srcPos[0].x, &srcDir[0].x, dt);
    weight[0] = SRC_VT(cam + 0x68, 0x14, SrcValueFn)(cam + 0x68);
    value[0]  = SRC_VT(cam + 0x68, 0x18, SrcValueFn)(cam + 0x68);
    s32 n = 1;
    for (s32 zi = 0; zi <= *(s32*)(*(u8**)gWorldState + 0x4); zi++) {
        if (!ZoneIsLoaded(gWorldState, zi)) {
            continue;
        }
        u8* res = *(u8**)(*(u8**)gWorldState + 0x1A4) + zi * gZoneResourceSize;
        for (s32 i = 0; i < *(u8*)(res + 0xAC); i++) {
            void* src = (*(void***)(res + 0xA8))[i];
            if (src == NULL) {
                continue;
            }
            s32 applies = SRC_VT(src, 0xC, SrcGetFn)(src, &srcPos[n].x, &srcDir[n].x, dt);
            if (applies != 0) {
                *(u8*)(cam + 0x19) = 1;
            }
            if (n >= 10) {
                break;
            }
            if (applies != 0) {
                weight[n] = SRC_VT(src, 0x14, SrcValueFn)(src);
                value[n]  = SRC_VT(src, 0x18, SrcValueFn)(src);
                n++;
            }
        }
    }
    f32 most = 0.0f;
    for (s32 i = 1; i < n; i++) {
        if (weight[i] > most) most = weight[i];
    }
    f32 own = 1.0f - most;
    if (own < 0.0f) own = 0.0f;
    else if (own > 1.0f) own = 1.0f;
    weight[0] = own;
    f32 total = 0.0f;
    f32 valueSum = 0.0f;
    for (s32 i = 0; i < n; i++) {
        f32 w = weight[i];
        total += w;
        pos[0] += srcPos[i].x * w;
        valueSum += value[i];
        pos[1] += srcPos[i].y * w;
        pos[2] += srcPos[i].z * w;
        dir[0] += srcDir[i].x * w;
        dir[1] += srcDir[i].y * w;
        dir[2] += srcDir[i].z * w;
    }
    f32 inv = 1.0f / total;
    *blend = valueSum / (f32)n;
    pos[0] *= inv;
    pos[1] *= inv;
    pos[2] *= inv;
    Vec3 d = { dir[0], dir[1], dir[2] };
    d = Vec3Normalized(d);
    dir[0] = d.x;
    dir[1] = d.y;
    dir[2] = d.z;
}
