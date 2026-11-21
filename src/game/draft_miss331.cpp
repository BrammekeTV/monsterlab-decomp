// Spike / lightning helper objects around 0x800D02C0 and 0x800ED944.
#include "types.h"
#include <math.h>

extern "C" {
u32   RandomU32(void);                                                      // 0x80143460
void* SpawnHelper(void* owner, s32 arg);                                    // 0x8000C478
int   SysSprintf(char* buf, const char* fmt, ...);                          // 0x80142EF8
s32   fn_80065200(u8* arg0, u8* arg1, void* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6);   // 0x80065200
u8*   fn_8006FA74(u8* self, s32 n);                                         // 0x8006FA74 (base ctor, returns self)
void  fn_800D0DC0(u8* p);                                                   // 0x800D0DC0
void  fn_801B55F4(u8* obj, u8* out, s32 idx);                               // 0x801B55F4
void* fn_801B7198(void* anim, s32 a);                                       // 0x801B7198
u8*   fn_801BE620(u8* list, s32 a, u8* b, u8* c, s32 d, s32 e);             // 0x801BE620 (spawns a list entry, returns it)
void  fn_801BF398(u8* list, u8* item);                                      // 0x801BF398 (removes an entry from the list)
s32   fn_801ED368(u8* ref);                                                 // 0x801ED368
s32   fn_801ED538(u8* ref);                                                 // 0x801ED538
void  fn_801ED788(f32* out, u8* ref);                                       // 0x801ED788
u8*   fn_801F61AC(u8* arg0, s32 arg1);                                      // 0x801F61AC
u8*   fn_801F6848(u8* arg0, s32 arg1);                                      // 0x801F6848
void* fn_801F70AC(u8* p, s32 a);                                            // 0x801F70AC
u8*   fn_801F7800(u8* p, s32 a4);                                           // 0x801F7800
u8*   fn_801F8110(u8* p, s32 a4);                                           // 0x801F8110
void  fn_801F8238(u8* p, s32 a4);                                           // 0x801F8238
void  fn_801F94B0(u8* p, s32 v);                                            // 0x801F94B0
u8*   fn_801F9544(u8* p, s32 a4);                                           // 0x801F9544
u8*   fn_801F9C2C(u8* p, s32 a4);                                           // 0x801F9C2C
u8*   fn_801F9D3C(u8* p, s32 a);                                            // 0x801F9D3C
u8*   fn_801FA884(u8* p, s32 a4);                                           // 0x801FA884
void  fn_801FAAE0(u8* p, s32 a);                                            // 0x801FAAE0
}

extern "C++" u8* fn_801B55A8(u8* ent);   // 0x801B55A8 (C++ linkage; returns the locator count as an integer value)

extern u8 lbl_8035F898[];   // 0x8035F898 (spike names "Multi Dir Spike", "Axial Dir Spike", ...; 0x10 apart)
extern u8 lbl_8035FB30[];   // 0x8035FB30 (vtable)
extern u8 lbl_803C4298[];   // 0x803C4298 (level info block, +0x38 passed to the sub-object constructors)
extern u8 lbl_8047C790[];   // 0x8047C790 (effect entry list)

static const f32 kU32ToUnit = 2.3283064e-10f;   // 1 / 2^32

// Normalizes (x, y, z) into out; a zero-length vector gives (0, 0, 0).
static void Normalize3(f32* out, f32 x, f32 y, f32 z) {
    f32 len2 = (z * z) + ((x * x) + (y * y));
    if (len2 == 0.0f) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 0.0f;
    } else {
        f32 inv = 1.0f / sqrtf(len2);
        out[0] = x * inv;
        out[1] = y * inv;
        out[2] = z * inv;
    }
}

// Reads a locator's 12 floats (+0x38..+0x64 of a 0xC8-byte record, or from the object's locator
// table) for the reference stored at p+off (flags u16, index byte at off+2). False if invalid.
static bool FetchLocator(u8* world, u8* p, s32 off, f32* out) {
    u16 fl = *(u16*)(p + off);
    s32 idx = (fl & 1) ? *(u8*)(p + off + 2) : -1;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(world)) {
            fn_801B55F4(world, (u8*)out, idx);
            return true;
        }
        return false;
    }
    idx = (fl & 2) ? *(u8*)(p + off + 2) : -1;
    if (idx == -1) return false;
    u8* rec = *(u8**)(world + 0x98) + idx * 0xC8;
    for (s32 i = 0; i < 12; i++) out[i] = *(f32*)(rec + 0x38 + i * 4);
    return true;
}

// Refreshes the beam's start (+0x10) and end (+0x1C) points from the things it is attached to.
// Returns 2 = keep going, 3 = finished.
static s32 UpdateBeamEnds(u8* p) {
    u8* world = *(u8**)(p + 0x68);
    if (world != NULL && (*(u32*)(*(u8**)(world + 4) + 0x20) & 3) == 0) return 3;

    u32 flags = *(u32*)(p + 0x50);
    f32 v[3];
    if (flags & 8) {
        // Both ends come from two references (+8 and +0xC).
        if (!fn_801ED368(p + 8)) return 3;
        if (!fn_801ED368(p + 0xC)) return 3;
        fn_801ED788(v, p + 8);
        *(f32*)(p + 0x10) = v[0];
        *(f32*)(p + 0x14) = v[1];
        *(f32*)(p + 0x18) = v[2];
        fn_801ED788(v, p + 0xC);
        *(f32*)(p + 0x1C) = v[0];
        *(f32*)(p + 0x20) = v[1];
        *(f32*)(p + 0x24) = v[2];
        return 2;
    }
    if (world == NULL) return 3;

    f32 loc[12] = {};
    if (!FetchLocator(world, p, 0, loc)) return 3;
    *(f32*)(p + 0x10) = loc[9];
    *(f32*)(p + 0x14) = loc[10];
    *(f32*)(p + 0x18) = loc[11];

    flags = *(u32*)(p + 0x50);
    if (flags & 2) {
        // End point from a second locator.
        if (!FetchLocator(world, p, 4, loc)) return 3;
        *(f32*)(p + 0x1C) = loc[9];
        *(f32*)(p + 0x20) = loc[10];
        *(f32*)(p + 0x24) = loc[11];
        return 2;
    }
    if (!(flags & 4)) return 3;

    // End point from a reference (+0xC), nudged by the target's height and a random sideways jitter.
    if (!fn_801ED368(p + 0xC)) return 3;
    fn_801ED788(v, p + 0xC);
    *(f32*)(p + 0x1C) = v[0];
    *(f32*)(p + 0x20) = v[1];
    *(f32*)(p + 0x24) = v[2];

    s32 id = fn_801ED538(p + 0xC);
    u8* target = (u8*)fn_801B7198(&id, 0);
    if (target != NULL && (*(u32*)(*(u8**)(target + 4) + 0x1C) & 0x10)) {
        f32 drop = -*(f32*)(target + 0x7D4);
        *(f32*)(p + 0x1C) = *(f32*)(p + 0x1C) + 0.0f;
        *(f32*)(p + 0x20) = *(f32*)(p + 0x20) + 0.0f;
        *(f32*)(p + 0x24) = *(f32*)(p + 0x24) + drop;
    }

    f32 spread = *(f32*)(p + 0x3C);
    if (spread > 0.0f) {
        f32 jx = 0.1f + 0.9f * (kU32ToUnit * (f32)RandomU32());
        f32 jy = 0.1f + 0.9f * (kU32ToUnit * (f32)RandomU32());
        switch (RandomU32() & 3) {
        case 1: jx = -jx; break;
        case 2: jy = -jy; break;
        case 3: jx = -jx; jy = -jy; break;
        default: break;
        }
        f32 n[3];
        Normalize3(n, jx, jy, 0.0f);
        f32 dx = n[0] * spread;
        f32 dy = n[1] * spread;
        f32 dz = n[2] * spread;
        *(f32*)(p + 0x1C) = *(f32*)(p + 0x1C) + dx;
        *(f32*)(p + 0x20) = *(f32*)(p + 0x20) + dy;
        *(f32*)(p + 0x24) = *(f32*)(p + 0x24) + dz;
    }
    return 2;
}

// Removes the beam's list entry (+0x40) if it has one.
static void DropBeamEntry(u8* p) {
    u8* ent = *(u8**)(p + 0x40);
    if (ent != NULL) {
        fn_801BF398(lbl_8047C790, ent);
        *(u8**)(p + 0x40) = NULL;
    }
}

extern "C" {

// 0x800D02C0
// Constructor of the spike-pit object: base ctor, vtable, smears the byte at +0x8E over +0x8C/+0x8D and
// every triple in +0x8F..+0x17C, builds 2 x 11 sub-objects (+0x1D0..), six named spike slots (each
// with a zeroed position/direction and its name written by SysSprintf) and the six axis directions
// (+0x188..+0x1C4). Returns the object.
u8* fn_800D02C0(u8* self) {
    fn_8006FA74(self, 2);

    u8 b = *(u8*)(self + 0x8E);
    *(u8**)(self + 0x4) = lbl_8035FB30;
    *(u8*)(self + 0x8D) = b;
    *(u8*)(self + 0x8C) = b;

    // Every triple takes the value of its last byte (the original unrolls this by 8).
    u8* end = self + 0x17C;
    for (u8* q = self + 0x8F; q < end; q += 3) {
        u8 v = q[2];
        q[1] = v;
        q[0] = v;
    }

    *(s32*)(self + 0x180) = -1;
    *(s32*)(self + 0x184) = -1;

    s32 info = *(s32*)(lbl_803C4298 + 0x38);
    fn_801F9D3C(self + 0x1D0, info);
    fn_801FA884(self + 0x2D0, info);
    fn_801FAAE0(self + 0x384, info);
    fn_801F8238(self + 0x444, info);
    fn_801F8110(self + 0x8C8, info);
    fn_801F94B0(self + 0xD4C, info);
    fn_801F7800(self + 0x11D0, info);
    fn_801F9544(self + 0x12F4, info);
    fn_801F9C2C(self + 0x13A8, info);
    fn_801F70AC(self + 0x1464, info);
    fn_801F6848(self + 0x150C, info);
    fn_801F9D3C(self + 0x15CC, info);
    fn_801FA884(self + 0x16CC, info);
    fn_801FAAE0(self + 0x1780, info);
    fn_801F8238(self + 0x1840, info);
    fn_801F8110(self + 0x1CC4, info);
    fn_801F94B0(self + 0x2148, info);
    fn_801F7800(self + 0x25CC, info);
    fn_801F9544(self + 0x26F0, info);
    fn_801F9C2C(self + 0x27A4, info);
    fn_801F70AC(self + 0x2860, info);
    fn_801F6848(self + 0x2908, info);
    fn_801F61AC(self + 0x29C8, info);

    // Six spike slots: a name string at the slot start (written by SysSprintf), five zeroed floats at
    // +0x200 and a zeroed int at +0x214; the three names repeat (offsets 0, 0x10, 0x20 in lbl_8035F898).
    static const s32 kSlotBase[6] = {0x2B84, 0x2E9C, 0x31B4, 0x34CC, 0x37E4, 0x3AFC};
    static const s32 kNameOff[6] = {0x0, 0x10, 0x20, 0x0, 0x10, 0x20};
    for (s32 i = 0; i < 6; i++) {
        u8* slot = self + kSlotBase[i];
        *(s32*)(slot + 0x214) = 0;
        *(f32*)(slot + 0x200) = 0.0f;
        *(f32*)(slot + 0x204) = 0.0f;
        *(f32*)(slot + 0x208) = 0.0f;
        *(f32*)(slot + 0x20C) = 0.0f;
        *(f32*)(slot + 0x210) = 0.0f;
        SysSprintf((char*)slot, (const char*)(lbl_8035F898 + kNameOff[i]));
    }

    // The six axis directions: -Y, +Y, +X, -X, -Z, +Z (normalized).
    static const f32 kAxes[6][3] = {
        {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f},
    };
    static const s32 kAxisOff[6] = {0x188, 0x194, 0x1A0, 0x1AC, 0x1B8, 0x1C4};
    for (s32 i = 0; i < 6; i++) {
        f32 n[3];
        Normalize3(n, kAxes[i][0], kAxes[i][1], kAxes[i][2]);
        *(f32*)(self + kAxisOff[i] + 0) = n[0];
        *(f32*)(self + kAxisOff[i] + 4) = n[1];
        *(f32*)(self + kAxisOff[i] + 8) = n[2];
    }

    fn_800D0DC0(self);
    return self;
}

// 0x800ED944
// Per-frame update of a lightning/beam effect (callback of the effect entry); state byte at +0x7F:
// 0x08 keep, 0x10 follow the endpoints and spawn the beam entry, 0x20 follow the endpoints with a
// periodic re-spawn (and a hit on the target), 0x40 remove the entry, 0x80/other finished.
// Returns 2 (keep updating) or 3 (finished).
s32 fn_800ED944(u8* p, f32 dt) {
    switch (*(u8*)(p + 0x7F)) {
    case 0x08:
        return 2;

    case 0x10: {
        s32 status = UpdateBeamEnds(p);
        if (status != 3) {
            u8* ent = fn_801BE620(lbl_8047C790, *(s32*)(p + 0x48), p + 0x10, p + 0x1C, 0, 0);
            *(u8**)(p + 0x40) = ent;
            if (ent == NULL) status = 3;
        } else {
            *(u8**)(p + 0x40) = NULL;
        }
        return status;
    }

    case 0x20: {
        u32 flags = *(u32*)(p + 0x50);
        if (!(flags & 1) && *(f32*)(p + 0x70) >= *(f32*)(p + 0x28)) {
            // Strike: hit the target along the beam.
            void* owner = *(void**)(p + 0x44);
            if (owner != NULL) {
                void* shot = SpawnHelper(owner, 0);
                if (shot != NULL) {
                    f32 dx = *(f32*)(p + 0x1C) - *(f32*)(p + 0x10);
                    f32 dy = *(f32*)(p + 0x20) - *(f32*)(p + 0x14);
                    f32 dz = *(f32*)(p + 0x24) - *(f32*)(p + 0x18);
                    f32 dir[3];
                    Normalize3(dir, dx, dy, dz);
                    fn_80065200((u8*)shot, p + 0x1C, dir, *(u8**)(p + 0x68), 0, 0, 0);
                }
            }
            if (*(f32*)(p + 0x38) != 0.0f || *(f32*)(p + 0x34) != 0.0f) {
                // Re-roll the time until the next respawn between +0x34 and +0x38 and drop the old entry.
                if (*(u8**)(p + 0x40) != NULL) {
                    fn_801BF398(lbl_8047C790, *(u8**)(p + 0x40));
                    *(u8**)(p + 0x40) = NULL;
                    f32 hi = *(f32*)(p + 0x38);
                    f32 lo = *(f32*)(p + 0x34);
                    *(f32*)(p + 0x2C) = lo + (hi - lo) * (kU32ToUnit * (f32)RandomU32());
                }
                f32 t = *(f32*)(p + 0x30) + dt;
                *(f32*)(p + 0x30) = t;
                if (t > *(f32*)(p + 0x2C)) {
                    *(f32*)(p + 0x30) = 0.0f;
                    *(f32*)(p + 0x70) = 0.0f;
                    *(u8**)(p + 0x40) = fn_801BE620(lbl_8047C790, *(s32*)(p + 0x48), p + 0x10, p + 0x1C, 0, 0);
                }
                return 2;
            }
            DropBeamEntry(p);
            return 3;
        }

        if (*(u8**)(p + 0x40) == NULL) return 3;
        if (UpdateBeamEnds(p) == 3) {
            DropBeamEntry(p);
            return 3;
        }
        // Move the live beam entry's first and last points to the new endpoints.
        u8* ent = *(u8**)(p + 0x40);
        *(f32*)(ent + 0x4C) = *(f32*)(p + 0x10);
        *(f32*)(ent + 0x50) = *(f32*)(p + 0x14);
        *(f32*)(ent + 0x54) = *(f32*)(p + 0x18);
        u8* ent2 = *(u8**)(p + 0x40);
        u8* last = ent2 + (*(s32*)(ent2 + 0x38) - 1) * 0xC;
        *(f32*)(last + 0x4C) = *(f32*)(p + 0x1C);
        *(f32*)(last + 0x50) = *(f32*)(p + 0x20);
        *(f32*)(last + 0x54) = *(f32*)(p + 0x24);
        return 2;
    }

    case 0x40:
        DropBeamEntry(p);
        return 3;

    default:
        return 3;
    }
}

}
