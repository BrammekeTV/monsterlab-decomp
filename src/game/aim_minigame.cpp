// Aiming mini-game: a tool on a turntable locator is rotated with the pointer; a ray from it
// is tested against a target (circle of radius 0.5), and the remote rumbles while it is on
// target. A hit test against the scene also toggles a marker where the ray lands.
// Original: AimMinigame_Update 0x8007FE30 (auto_03_8007D4DC_text.s)
#include <math.h>
#include <stddef.h>

#include "types.h"
#include "engine/math.h"
#include "game/script_callbacks.h"

extern "C" {
void MtxFromLocatorRotation(f32 out[12], const void* rot);        // 0x80143CA8
void QuatFromMtx(Quat* out, const f32 m[12]);                      // 0x801456E4
void QuatFromAxisAngle(Quat* out, const Vec3* axis, f32 angle);    // 0x80145EE8
void QuatGetAxisXY(const Quat* q, Vec3* out);                      // 0x801453BC
s32  RaySceneTest(void* ray);                                      // 0x80070458
}

extern u8 gWorldState[];      // 0x803E6D00
extern u8 gScriptCtx[];       // 0x803C4298 (+0x38 = controller)

static const s32 kHitCounts[4] = { 1, 1, 0, 0 };   // 0x803464B8: result kinds that count as a hit
#define TURNTABLE_LOCATOR 0xAAE1A370
#define TARGET_RADIUS_SQ  0.25f

// Locator record in the world data (+0x28, 0x20 bytes each): hash, pos, rotation, angle.
static u8* FindLocator(u32 hash) {
    u8* data = *(u8**)gWorldState;
    u32 n = *(u32*)(data + 0x24);
    s32 found = -1;
    for (u32 i = 0; i < n; i++) {
        if (*(u32*)(*(u8**)(data + 0x28) + i * 0x20) == hash) {
            found = (s32)i;
            break;
        }
    }
    if (found >= 0 && found < *(s32*)(data + 0x24)) return *(u8**)(data + 0x28) + found * 0x20;
    return NULL;
}

// out = a * m for 3x4 row-vector matrices (rotation rows 0..2, translation row 3).
static void MtxConcat(f32 out[12], const f32 a[12], const f32 m[12]) {
    for (s32 r = 0; r < 3; r++)
        for (s32 c = 0; c < 3; c++)
            out[r * 3 + c] = a[r * 3 + 2] * m[6 + c] + (a[r * 3 + 0] * m[0 + c] + a[r * 3 + 1] * m[3 + c]);
    for (s32 c = 0; c < 3; c++)
        out[9 + c] = m[9 + c] + (a[11] * m[6 + c] + (a[9] * m[0 + c] + a[10] * m[3 + c]));
}

static inline void MarkMoved(u8* def) {
    *(u32*)(def + 0x20) |= 0x8000;
    *(u32*)(def + 0x20) |= 0x10000000;
    *(u32*)(def + 0x20) |= 0x4000;
    *(u32*)(def + 0x20) |= 0x10000000;
}

static inline void SetVisualFlag(u8* e, BOOL on) {
    typedef u8* (*GetFn)(u8*);
    GetFn getVisual = *(GetFn*)(*(u8**)e + 0xC);
    if (getVisual(e) != NULL) {
        u32* f = (u32*)(getVisual(e) + 8);
        *f = on ? (*f | 2) : (*f & ~2u);
    }
    u32* sf = (u32*)(*(u8**)(e + 4) + 0x20);
    *sf = on ? (*sf | 2) : (*sf & ~2u);
}

static inline Quat QuatMul(const Quat& a, const Quat& b) {
    Quat r;
    r.x = ((a.y * b.z) + ((a.w * b.x) + (a.x * b.w))) - (a.z * b.y);
    r.y = (a.z * b.x) + ((a.y * b.w) + ((a.w * b.y) - (a.x * b.z)));
    r.z = (a.z * b.w) + (((a.w * b.z) + (a.x * b.y)) - (a.y * b.x));
    r.w = (((a.w * b.w) - (a.x * b.x)) - (a.y * b.y)) - (a.z * b.z);
    return r;
}

extern "C" void AimMinigame_Update(u8* g);
// 0x8007FE30
void AimMinigame_Update(u8* g) {
    u8* table = FindLocator(TURNTABLE_LOCATOR);
    *(f32*)(table + 0x1C) = 0.63661975f * *(f32*)(g + 0xA8);   // pointer angle -> turns
    f32 m[12];
    MtxFromLocatorRotation(m, table + 0x14);
    m[9] += *(f32*)(table + 0x8);
    m[10] += *(f32*)(table + 0xC);
    m[11] += *(f32*)(table + 0x10);

    // Tool transform = attachment offset (g+0x2BC) * turntable.
    f32 tool[12];
    MtxConcat(tool, (const f32*)(g + 0x2BC), m);
    f32* nodePos = (f32*)(*(u8**)(*(u8**)(g + 0x8C) + 0x58) + 8);
    nodePos[0] = tool[9];
    nodePos[1] = tool[10];
    nodePos[2] = tool[11];

    // The base entity sits on the turntable and turns with it.
    u8* baseDef = *(u8**)(*(u8**)(g + 0x1F4) + 4);
    ((f32*)baseDef)[0] = m[9];
    ((f32*)baseDef)[1] = m[10];
    ((f32*)baseDef)[2] = 2.0f;
    MarkMoved(baseDef);
    Quat q, spin;
    QuatFromMtx(&q, m);
    Vec3 up = { 0.0f, 0.0f, 1.0f };
    QuatFromAxisAngle(&spin, &up, 2.0f);
    q = QuatMul(q, spin);
    baseDef = *(u8**)(*(u8**)(g + 0x1F4) + 4);
    *(Quat*)(baseDef + 0xC) = q;
    MarkMoved(baseDef);

    // Muzzle: local offset g+0x2B0 through the tool transform.
    const f32* ofs = (const f32*)(g + 0x2B0);
    baseDef = *(u8**)(*(u8**)(g + 0x1F4) + 4);
    ((f32*)baseDef)[0] = tool[9] + (tool[6] * ofs[2] + (tool[0] * ofs[0] + tool[3] * ofs[1]));
    ((f32*)baseDef)[1] = tool[10] + (tool[7] * ofs[2] + (tool[1] * ofs[0] + tool[4] * ofs[1]));
    ((f32*)baseDef)[2] = tool[11] + (tool[8] * ofs[2] + (tool[2] * ofs[0] + tool[5] * ofs[1]));
    MarkMoved(baseDef);

    // Ray from the muzzle, flat, `g+0xB4` long.
    u8* ray = **(u8***)(g + 0x31C);
    f32* start = (f32*)(ray + 0xC);
    start[0] = ((f32*)baseDef)[0];
    start[1] = ((f32*)baseDef)[1];
    start[2] = ((f32*)baseDef)[2];
    start[2] = 0.25f;
    Vec3 dir;
    QuatGetAxisXY((const Quat*)(*(u8**)(*(u8**)(g + 0x1F4) + 4) + 0xC), &dir);
    dir.z = 0.0f;
    f32 len = *(f32*)(g + 0xB4);
    f32* end = (f32*)(ray + 0x34);
    end[0] = start[0] - dir.x * len;
    end[1] = start[1] - dir.y * len;
    end[2] = start[2] - 0.0f * len;

    u8* marker = *(u8**)(g + 0x2A4);
    if (kHitCounts[RaySceneTest(ray)] != 0) {
        *(u32*)(*(u8**)(g + 0x31C) + 0x30) &= ~1u;
        SetVisualFlag(marker, TRUE);
        ray = **(u8***)(g + 0x31C);
        u8* mdef = *(u8**)(marker + 4);
        ((f32*)mdef)[0] = ((f32*)(ray + 0xC))[0];
        ((f32*)mdef)[1] = ((f32*)(ray + 0xC))[1];
        ((f32*)mdef)[2] = 0.01f + ((f32*)(ray + 0xC))[2];
        MarkMoved(mdef);
        marker = *(u8**)(g + 0x2A4);
        mdef = *(u8**)(marker + 4);
        *(Quat*)(mdef + 0xC) = *(Quat*)(*(u8**)(*(u8**)(g + 0x1F4) + 4) + 0xC);
        MarkMoved(mdef);
    } else {
        SetVisualFlag(marker, FALSE);
        *(u32*)(*(u8**)(g + 0x31C) + 0x30) &= ~1u;
    }

    // Does the ray line cross the target circle?
    ray = **(u8***)(g + 0x31C);
    start = (f32*)(ray + 0xC);
    end = (f32*)(ray + 0x34);
    Vec3 d = { end[0] - start[0], end[1] - start[1], end[2] - start[2] };
    d = Vec3Normalized(d);
    const f32* target = (const f32*)*(u8**)(*(u8**)(g + 0x29C) + 4);
    f32 a = d.x * d.x + d.y * d.y;
    f32 b = 2.0f * (d.x * (start[0] - target[0]) + d.y * (start[1] - target[1]));
    f32 c = (target[1] * target[1] + (target[0] * target[0] + (start[0] * start[0] + start[1] * start[1]))) -
            (TARGET_RADIUS_SQ + 2.0f * (start[0] * target[0] + start[1] * target[1]));
    f32 disc = b * b - 4.0f * a * c;
    BOOL onTarget = disc >= 0.0f;

    s32 chan = *(s32*)(gScriptCtx + 0x38);
    if (onTarget && !g[0x320])      StartRumble(chan, 0);
    else if (g[0x320] && !onTarget) StopRumble(chan);
    g[0x320] = (u8)onTarget;
}
