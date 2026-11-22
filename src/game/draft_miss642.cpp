// Mallet minigame style battle helpers: target spawn placement, slot animation, slot picking,
// attack start debug trace and a dispatch-table reset.
#include <string.h>

#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*ObjVFn)(u8*, s32);            // vtable slot 0x2C: (object, message code)
typedef void (*ObjVFn0)(u8*);                // vtable slot 0x64
typedef void (*OverlaySetFn)(void*, s32, u32);   // lbl_80348F38 + type*4 + 0x24
typedef s32 (*OverlayGetFn)(void*, s32);         // lbl_80348F38 + type*4 + 0x30

extern "C" {
void* AbilityEffect(void* db, s32 ability);                      // 0x8021679C
void* ChoiceDestroy(void* c, s32 del);                           // 0x8020F394
u32   EffectEmitter(void* db, s32 effect, s32 limb);             // 0x800523B8
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);   // 0x80209E70
u32   RandomU32(void);                                           // 0x80143460
s32   RoundGetAttacker(void* round);                             // 0x80214F48
void* ScreenSetFindElement(void* set, u32 hash);                 // 0x80198F18
void* ScreenSetsCurrent(void* sets);                             // 0x8018D1E4
int   SysSprintf(char* buf, const char* fmt, ...);               // 0x80142EF8
void  fn_800223DC(u8* p);                                        // 0x800223DC
s32   fn_80031114(void* list, f32* v);                           // 0x80031114
u8*   fn_8003CF14(u8* p);                                        // 0x8003CF14 (returns *(p+8) + 0x68)
void* fn_8003F7DC(void);                                         // 0x8003F7DC (returns an id)
s32   fn_8003F8D4(void);                                         // 0x8003F8D4
void  fn_80041810(u8* dst, u8* src);                             // 0x80041810
void* fn_801D40E0(s32 n);                                        // 0x801D40E0
void* fn_80209814(void* v);                                      // 0x80209814

extern const u32 kAbilityEffects[];   // 0x80388258
extern char lbl_803C5358[];           // 0x803C5358 (debug text buffer)
extern const char lbl_80358C68[];     // 0x80358C68 (debug format strings)
extern const char lbl_805F8698[];     // 0x805F8698 (debug format string)
extern u8 lbl_80348F38[];             // 0x80348F38 (overlay dispatch table)
extern u8 lbl_803C4298[];             // 0x803C4298
extern void* lbl_805FBBAC;            // 0x805FBBAC (voice SFX handle)

extern f32 lbl_805FCF00;   // 0.0
extern f32 lbl_805FCF5C;   // 1.0
extern f32 lbl_805FCF84;   // u32 -> [0,1) scale
extern f32 lbl_805FCF88;   // 4.0
extern f32 lbl_805FCFAC;   // 0.0
extern f32 lbl_805FCFB8;
extern f32 lbl_805FCFD0;   // 0.4
extern f32 lbl_805FCFEC;   // 1.0
extern f32 lbl_805FCFF4;   // -1000.0 (element parked off screen)
extern f32 lbl_805FD008;   // 10.0
extern f32 lbl_805FD00C;   // 2.1
extern f32 lbl_805FD010;   // 85.0
extern f32 lbl_805FD014;   // 0.2
extern f64 lbl_805FD028;   // 0.01
extern f32 lbl_805FD030;   // 0.1
extern f32 lbl_805FD034;
extern f32 lbl_805FD228;   // 0.0
extern f32 lbl_805FD22C;   // 0.25
extern f32 lbl_805FD230;   // 2.0
extern f32 lbl_805FD274;   // 1.0
}

extern u8* gBattle;          // 0x805FBBB8
extern u8* gGameDb;          // 0x805FBBC0
extern "C++" void* gScreenSets;   // 0x805FB654

static const u32 kHashMallet = 0x2EDD7464u;  // "Mallet"

static inline f32 RandUnit(void) {
    return lbl_805FCF84 * (f32)RandomU32();
}

static inline u32 F32Bits(f32 f) {
    u32 r;
    memcpy(&r, &f, 4);
    return r;
}

// Creates the voice SFX handle once.
static inline void EnsureVoiceHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) h = fn_80209814(h);
        lbl_805FBBAC = h;
    }
}

// True when `p` keeps its distance (the squared limit at +0x6B0) from every placed spawn point.
static bool SpawnPointFree(u8* arg0, const f32* p) {
    for (s32 g = 0; g < 4; g++) {
        u8* grp = arg0 + g * 0x28;
        s32 count = *(s32*)(grp + 0x5FC);
        for (s32 i = 0; i < count; i++) {
            u8* e = *(u8**)(grp + 0x5F8) + i * 0x10;
            f32 dx = p[0] - *(f32*)(e + 0x0);
            f32 dy = p[1] - *(f32*)(e + 0x4);
            f32 dz = p[2] - *(f32*)(e + 0x8);
            f32 d2 = dz * dz + (dx * dx + dy * dy);
            if (d2 < *(f32*)(arg0 + 0x6B0)) return false;
        }
    }
    return true;
}

extern "C" {

// 0x80039774
// Binds the four screen elements, then scatters random spawn points for each of the four zones
// (rejection sampling against the points already placed, at most 1000 tries per point).
void fn_80039774(u8* arg0) {
    fn_800223DC(arg0);
    void* set = ScreenSetsCurrent(gScreenSets);
    *(void**)(arg0 + 0x90) = ScreenSetFindElement(set, kHashMallet);
    *(void**)(arg0 + 0x98) = ScreenSetFindElement(set, 0x21570E76u);
    *(void**)(arg0 + 0x9C) = ScreenSetFindElement(set, 0xDC6938E9u);
    // (the original switched on *(s32*)(arg0 + 0x5E8) here with no effect)
    *(void**)(arg0 + 0xA0) = ScreenSetFindElement(set, 0x6EBC6ED5u);

    for (s32 z = 0; z < 4; z++) {
        u8* zone = arg0 + z * 0x28;
        s32 want = (s32)(lbl_805FCF5C + lbl_805FCF00 * RandUnit());
        s32 placed = 0;
        s32 off = 0;
        while (placed < want) {
            f32 cand[3];
            s32 tries = 0;
            cand[0] = lbl_805FCF00;
            cand[1] = lbl_805FCF00;
            cand[2] = lbl_805FCF00;
            for (;;) {
                f32 loX = *(f32*)(zone + 0x604);
                f32 hiX = *(f32*)(zone + 0x60C);
                f32 x = loX + (hiX - loX) * RandUnit();
                f32 loY = *(f32*)(zone + 0x608);
                f32 hiY = *(f32*)(zone + 0x610);
                f32 y = loY + (hiY - loY) * RandUnit();
                cand[0] = x;
                cand[1] = y;
                if (fn_80031114(zone + 0x5EC, cand) != 0 && SpawnPointFree(arg0, cand)) break;
                tries++;
                if (tries >= 1000) break;
            }
            if (tries >= 1000) break;
            u8* pt = *(u8**)(zone + 0x5F8) + off;
            *(f32*)(pt + 0x0) = cand[0];
            *(f32*)(pt + 0x4) = cand[1];
            *(f32*)(pt + 0x8) = cand[2];
            f32 r = RandUnit();
            placed++;
            pt = *(u8**)(zone + 0x5F8) + off;
            off += 0x10;
            *(s32*)(pt + 0xC) = (s32)(lbl_805FCF88 + lbl_805FCF00 * r);
            *(s32*)(zone + 0x5FC) = *(s32*)(zone + 0x5FC) + 1;
            *(s32*)(zone + 0x600) = *(s32*)(zone + 0x600) + 1;
        }
        *(s32*)(arg0 + 0x5E4) = *(s32*)(arg0 + 0x5E4) + *(s32*)(zone + 0x5FC);
    }
}

// 0x8003AEF4
// Per-frame update of the six slot elements (pairs at +0x1FC and +0x214): the active ones slide
// along their velocity until their timer runs out and are then parked; the idle ones grow/shrink
// toward the slot, play the hit sound when the timer completes and rumble the pad.
void fn_8003AEF4(u8* arg0, f32 dt) {
    for (s32 i = 0; i < 6; i++) {
        u8* p4 = arg0 + i * 4;
        u8* p12 = arg0 + i * 12;
        u8* b = *(u8**)(p4 + 0x214);
        if (*(f32*)(b + 0x68) > lbl_805FCFF4) {
            f32 t = *(f32*)(p12 + 0x2A8) + dt;
            *(f32*)(p12 + 0x2A8) = t;
            if (t >= *(f32*)(p4 + 0x348)) {
                *(f32*)(*(u8**)(p4 + 0x214) + 0x68) = lbl_805FCFF4;
            } else {
                b = *(u8**)(p4 + 0x214);
                if (t > lbl_805FCFAC) {
                    f32 x = *(f32*)(b + 0x68) + dt * *(f32*)(p12 + 0x2A0);
                    f32 y = *(f32*)(b + 0x6C) + dt * *(f32*)(p12 + 0x2A4);
                    f32 size[3];
                    *(f32*)(b + 0x68) = x;
                    *(f32*)(b + 0x6C) = y;
                    *(f32*)(b + 0x70) = lbl_805FD008;
                    u8* a = *(u8**)(p4 + 0x1FC);
                    size[0] = lbl_805FD00C * *(f32*)(a + 0x74);
                    size[1] = lbl_805FD00C * *(f32*)(a + 0x78);
                    size[2] = lbl_805FD00C * *(f32*)(a + 0x7C);
                    UIElement_SetSize(**(u32**)(p4 + 0x214), size);
                }
            }
            b = *(u8**)(p4 + 0x214);
            (*(ObjVFn*)(*(u8**)(b + 0x138) + 0x2C))(b, 8);
        } else {
            u8* a = *(u8**)(p4 + 0x1FC);
            if (*(f32*)(a + 0x68) > lbl_805FCFF4) {
                f32 t = *(f32*)(p12 + 0x2A8);
                u8* flag = arg0 + i + 0x294;
                bool done = false;
                if (*flag != 0) {
                    t = t - dt;
                    if (t <= lbl_805FCFAC) {
                        *(f32*)(a + 0x68) = lbl_805FCFF4;
                        *flag = 0;
                        *(f32*)(p12 + 0x2A8) = lbl_805FCFAC;
                        a = *(u8**)(p4 + 0x1FC);
                        (*(ObjVFn*)(*(u8**)(a + 0x138) + 0x2C))(a, 8);
                        *(s32*)(arg0 + 0x3A4) = *(s32*)(arg0 + 0x3A4) - 1;
                        done = true;
                    }
                } else {
                    t = t + dt;
                }
                if (!done) {
                    *(f32*)(p12 + 0x2A8) = t;
                    f32 limit = *(f32*)(p4 + 0x348);
                    if (t >= limit) {
                        EnsureVoiceHandle();
                        PlaySound(0xB36206DBu, 0, 0, 0, 0, 0);
                        (*(ObjVFn0*)(*(u8**)(arg0 + 4) + 0x64))(arg0);
                        a = *(u8**)(p4 + 0x1FC);
                        UIElement_SetPos(**(u32**)(p4 + 0x214), (f32*)(a + 0x68));
                        *(f32*)(*(u8**)(p4 + 0x1FC) + 0x68) = lbl_805FCFF4;
                        *(f32*)(p12 + 0x2A0) = lbl_805FCFAC;
                        *(f32*)(p12 + 0x2A4) = lbl_805FD010;
                        *(f32*)(p12 + 0x2A8) = -*(f32*)(arg0 + 0x954);
                        a = *(u8**)(p4 + 0x1FC);
                        f32 size[3];
                        size[0] = lbl_805FD00C * *(f32*)(a + 0x74);
                        size[1] = lbl_805FD00C * *(f32*)(a + 0x78);
                        size[2] = lbl_805FD00C * *(f32*)(a + 0x7C);
                        UIElement_SetSize(**(u32**)(p4 + 0x214), size);
                        b = *(u8**)(p4 + 0x214);
                        (*(ObjVFn*)(*(u8**)(b + 0x138) + 0x2C))(b, 8);
                        a = *(u8**)(p4 + 0x1FC);
                        (*(ObjVFn*)(*(u8**)(a + 0x138) + 0x2C))(a, 8);
                        *(s32*)(arg0 + 0x3A4) = *(s32*)(arg0 + 0x3A4) - 1;
                        TimedRumble(*(s32*)(lbl_803C4298 + 0x38), lbl_805FD014, 4);
                    } else {
                        a = *(u8**)(p4 + 0x1FC);
                        f32 x = *(f32*)(a + 0x68) + dt * *(f32*)(p12 + 0x2A0);
                        f32 y = *(f32*)(a + 0x6C) + dt * *(f32*)(p12 + 0x2A4);
                        f32 z = *(f32*)(a + 0x70);
                        *(f32*)(a + 0x68) = x;
                        *(f32*)(a + 0x6C) = y;
                        *(f32*)(a + 0x70) = z;
                        f32 lo = *(f32*)(arg0 + 0x378);
                        f32 hi = *(f32*)(arg0 + 0x37C);
                        f32 s = lo + (hi - lo) * (t / limit);
                        a = *(u8**)(p4 + 0x1FC);
                        *(f32*)(a + 0x74) = s;
                        *(f32*)(a + 0x78) = s;
                        *(f32*)(a + 0x7C) = lbl_805FCFEC;
                        a = *(u8**)(p4 + 0x1FC);
                        (*(ObjVFn*)(*(u8**)(a + 0x138) + 0x2C))(a, 8);
                    }
                }
            }
        }
    }
}

// 0x8003BC78
// Picks the next target: lists the free slots (7 entries of 0x34 bytes, flags at +0xBC/+0xBD),
// shuffles them, then either starts the hit on the chosen slot (mode 0) or the miss (mode 1);
// plays the failure sound when there is no current target.
void fn_8003BC78(u8* arg0) {
    s32 freeSlots[7];
    s32 n = 0;
    for (s32 i = 0; i < 7; i++) freeSlots[i] = 0;
    for (s32 i = 0; i < 7; i++) {
        u8* slot = arg0 + i * 0x34;
        if (*(u8*)(slot + 0xBC) == 0 && *(u8*)(slot + 0xBD) == 0) {
            freeSlots[n] = i;
            n++;
        }
    }
    if (n > 0) {
        for (s32 k = 0; k < 3; k++) {
            u32 a = RandomU32() % (u32)n;
            u32 b = RandomU32() % (u32)n;
            if ((s32)a != (s32)b) {
                s32 tmp = freeSlots[a];
                freeSlots[a] = freeSlots[b];
                freeSlots[b] = tmp;
            }
        }
    }
    if (*(s32*)(arg0 + 0x3D8) != 0 && *(s32*)(arg0 + 0x3E0) >= 0) {
        u8* cur = *(u8**)(arg0 + 0x924);
        *(s32*)(arg0 + 0x950) = 0;
        f32 cy = *(f32*)(cur + 0x6C);
        f32 cx = *(f32*)(cur + 0x68);
        // (the original selects the marker by a constant 0, so only the +0x944 marker is live)
        u8* marker = *(u8**)(arg0 + 0x944);
        *(f32*)(marker + 0x68) = cx;
        *(f32*)(marker + 0x6C) = cy;
        *(f32*)(marker + 0x70) = lbl_805FCFAC;
        marker = *(u8**)(arg0 + 0x944);
        (*(ObjVFn*)(*(u8**)(marker + 0x138) + 0x2C))(marker, 0x1A);
        TimedRumble(*(s32*)(lbl_803C4298 + 0x38), lbl_805FCFD0, 4);
        s32 mode = *(s32*)(arg0 + 0x3DC);
        if (mode == 0) {
            s32 idx = *(s32*)(arg0 + 0x3E0);
            s32 pick = freeSlots[0];
            *(u8*)(arg0 + idx + 0x29A) = 1;
            u8* slot = arg0 + pick * 0x34;
            *(s32*)(arg0 + *(s32*)(arg0 + 0x3E0) * 4 + 0x330) = pick;
            *(u8*)(slot + 0xBD) = 1;
            f32 speed = *(f32*)(arg0 + 0x390);
            u8* from = *(u8**)(arg0 + *(s32*)(arg0 + 0x3E0) * 4 + 0x244);
            u8* to = fn_8003CF14(slot + 0x90);
            f32 dx = *(f32*)(to + 0x0) - *(f32*)(from + 0x68);
            f32 dy = *(f32*)(to + 0x4) - *(f32*)(from + 0x6C);
            f32 dz = *(f32*)(to + 0x8) - *(f32*)(from + 0x70);
            u8* vel = arg0 + *(s32*)(arg0 + 0x3E0) * 0xC;
            *(f32*)(vel + 0x2E8) = dx / speed;
            *(f32*)(vel + 0x2EC) = dy / speed;
            *(f32*)(vel + 0x2F0) = dz / speed;
            vel = arg0 + *(s32*)(arg0 + 0x3E0) * 0xC;
            *(f32*)(vel + 0x2F0) = *(f32*)(arg0 + 0x390);
            EnsureVoiceHandle();
            PlaySound(0x9918C391u, 0, 0, 0, 0, 0);
        } else if (mode == 1) {
            EnsureVoiceHandle();
            PlaySound(0x6BC403E0u, 0, 0, 0, 0, 0);
            *(f32*)(arg0 + *(s32*)(arg0 + 0x3E0) * 0xC + 0x2A8) = *(f32*)(arg0 + 0x388);
        }
    } else {
        EnsureVoiceHandle();
        PlaySound(0x16CB5F04u, 0, 0, 0, 0, 0);
    }
}

// 0x8003C048
// Picks the slot element closest to the player marker (+0x924) among those that have just
// become ready: parks the six element pairs, then for each ready slot copies its position into
// the companion element and keeps the nearest one within reach (kind 1 = +0x1FC, kind 0 = +0x244)
// in +0x3D8/+0x3DC/+0x3E0.
void fn_8003C048(u8* arg0) {
    f32 zero = lbl_805FCFAC;
    f32 best = zero;
    *(u8**)(arg0 + 0x3D8) = NULL;
    *(s32*)(arg0 + 0x3DC) = -1;
    u8* player = *(u8**)(arg0 + 0x924);
    f32 px = *(f32*)(player + 0x68);
    f32 py = *(f32*)(player + 0x6C);
    f32 playerR = *(f32*)(player + 0x74);

    for (s32 i = 0; i < 6; i++) {
        u8* p4 = arg0 + i * 4;
        u8* p12 = arg0 + i * 12;
        u8* d = *(u8**)(p4 + 0x27C);
        *(f32*)(d + 0x68) = lbl_805FCFF4;
        u8* b = *(u8**)(p4 + 0x22C);
        *(f32*)(b + 0x68) = lbl_805FCFF4;
        d = *(u8**)(p4 + 0x27C);
        (*(ObjVFn*)(*(u8**)(d + 0x138) + 0x2C))(d, 8);
        b = *(u8**)(p4 + 0x22C);
        (*(ObjVFn*)(*(u8**)(b + 0x138) + 0x2C))(b, 8);

        if (*(u8*)(arg0 + i + 0x294) == 0 &&
            *(f32*)(p12 + 0x2A8) >= *(f32*)(arg0 + 0x394) * *(f32*)(p4 + 0x348)) {
            u8* a = *(u8**)(p4 + 0x1FC);
            b = *(u8**)(p4 + 0x22C);
            f32 ax = *(f32*)(a + 0x68);
            f32 ay = *(f32*)(a + 0x6C);
            f32 az = *(f32*)(a + 0x70);
            *(f32*)(b + 0x68) = ax;
            *(f32*)(b + 0x6C) = ay;
            *(f32*)(b + 0x70) = az;
            a = *(u8**)(p4 + 0x1FC);
            *(f32*)(*(u8**)(p4 + 0x22C) + 0x70) = (f32)((f64)*(f32*)(a + 0x70) - lbl_805FD028);
            a = *(u8**)(p4 + 0x1FC);
            u8* pl = *(u8**)(arg0 + 0x924);
            f32 ax2 = *(f32*)(a + 0x68);
            f32 ay2 = *(f32*)(a + 0x6C);
            f32 dx = ax2 - *(f32*)(pl + 0x68);
            f32 dy = ay2 - *(f32*)(pl + 0x6C);
            f32 dz = zero - zero;
            f32 reach = (*(f32*)(a + 0x74) + playerR) * lbl_805FCFB8;
            if (dz * dz + (dx * dx + dy * dy) <= reach * reach) {
                f32 ex = ax2 - px;
                f32 ey = ay2 - py;
                f32 d2 = zero * zero + (ex * ex + ey * ey);
                if (*(u8**)(arg0 + 0x3D8) == NULL || d2 < best) {
                    best = d2;
                    *(u8**)(arg0 + 0x3D8) = a;
                    *(s32*)(arg0 + 0x3DC) = 1;
                    *(s32*)(arg0 + 0x3E0) = i;
                }
            }
        }
        if (*(u8*)(arg0 + i + 0x29A) == 0 &&
            *(f32*)(p12 + 0x2F0) >= *(f32*)(arg0 + 0x394) * *(f32*)(p4 + 0x360)) {
            u8* c = *(u8**)(p4 + 0x244);
            d = *(u8**)(p4 + 0x27C);
            f32 cx = *(f32*)(c + 0x68);
            f32 cy = *(f32*)(c + 0x6C);
            f32 cz = *(f32*)(c + 0x70);
            *(f32*)(d + 0x68) = cx;
            *(f32*)(d + 0x6C) = cy;
            *(f32*)(d + 0x70) = cz;
            c = *(u8**)(p4 + 0x244);
            *(f32*)(*(u8**)(p4 + 0x27C) + 0x70) = *(f32*)(c + 0x70) - lbl_805FD030;
            c = *(u8**)(p4 + 0x244);
            u8* pl = *(u8**)(arg0 + 0x924);
            f32 cx2 = *(f32*)(c + 0x68);
            f32 cy2 = *(f32*)(c + 0x6C);
            f32 dx = cx2 - *(f32*)(pl + 0x68);
            f32 dy = cy2 - *(f32*)(pl + 0x6C);
            f32 dz = zero - zero;
            f32 reach = (lbl_805FD034 + playerR) * lbl_805FCFB8;
            if (dz * dz + (dx * dx + dy * dy) <= reach * reach) {
                f32 ex = cx2 - px;
                f32 ey = cy2 - py;
                f32 d2 = zero * zero + (ex * ex + ey * ey);
                if (*(u8**)(arg0 + 0x3D8) == NULL || d2 < best) {
                    best = d2;
                    *(u8**)(arg0 + 0x3D8) = c;
                    *(s32*)(arg0 + 0x3DC) = 0;
                    *(s32*)(arg0 + 0x3E0) = i;
                }
            }
        }
    }
}

// 0x8004AD24
// Attack start: sets the camera animation (boss or normal), records the attacker and, for each
// of the two fighters, looks up the chosen action/limb, flags which side reacts and spawns the
// ability particle effect on the limb. Most of the body is debug text output (SysSprintf).
void fn_8004AD24(u8* arg0, s32* arg1) {
    SysSprintf(lbl_803C5358, lbl_80358C68);
    *(f32*)(arg0 + 0x8) = lbl_805FD22C;
    if (fn_8003F8D4() != 0) {
        PlayBossCameraAnimation((u32)(uintptr_t)fn_8003F7DC());  // lint:ok id, not a pointer
    } else {
        PlayCameraAnimation((u32)(uintptr_t)fn_8003F7DC());  // lint:ok id, not a pointer
    }
    *(s32*)(arg0 + 0x10) = RoundGetAttacker(arg1);
    *(u8*)(arg0 + 0x14) = 1;
    if (*arg1 != 6) {
        for (s32 i = 0; i < 2; i++) {
            SysSprintf(lbl_803C5358, lbl_80358C68 + 0x10);
            u8* fighter = ((u8**)(gBattle + 0xA00))[i];
            SysSprintf(lbl_803C5358, lbl_80358C68 + 0x20, fighter);
            u8* choiceSrc = fighter + 0x20;
            SysSprintf(lbl_803C5358, lbl_80358C68 + 0x38, choiceSrc);
            s32 choice[3];
            fn_80041810((u8*)choice, choiceSrc);
            s32 limbIdx = choice[2];
            ChoiceDestroy(choice, -1);
            SysSprintf(lbl_803C5358, lbl_80358C68 + 0x38, choiceSrc);
            u8* limb = ((u8**)fighter)[limbIdx];
            if (limb == NULL) continue;
            s32 choice2[3];
            fn_80041810((u8*)choice2, choiceSrc);
            s32 actionIdx = choice2[0];
            ChoiceDestroy(choice2, -1);
            u8* db = *(u8**)(gGameDb + 0xC);
            s32 ability = *(s32*)(*(u8**)(limb + 0x28) + 0x20);
            u8* action = *(u8**)(db + 0x430) + actionIdx * 0x14;
            u8* type = *(u8**)(db + 0x8) + *(s32*)(action + 0x4) * 0x24;
            s32 statKey = *(s32*)(type + 0x8);
            if (statKey == 0 && *(u8*)(arg0 + 0x6) == 0 && ability == 0xF && *(s32*)(arg0 + 0x10) == i) {
                *(u8*)(arg0 + i + 0x4) = 1;
                *(u8*)(arg0 + 0x6) = 1;
            }
            if ((statKey == 1 || i == *(s32*)(arg0 + 0x10)) && ability == 0xA) {
                *(u8*)(arg0 + i + 0x4) = 1;
            }
            SysSprintf(lbl_803C5358, lbl_80358C68 + 0x50);
            if (*(u8*)(arg0 + i + 0x4) == 0) continue;
            if (*(f32*)(arg0 + 0x8) < lbl_805FD230) *(f32*)(arg0 + 0x8) = lbl_805FD230;
            if (ability == -1 || ability >= 0x10) continue;
            SysSprintf(lbl_803C5358, lbl_80358C68 + 0x60);
            u8* rec = *(u8**)(*(u8**)(gGameDb + 0xC) + 0x3610) + ability * 0x24;
            s32 effect = *(s32*)(rec + 0x10);
            if (effect == -1 || effect >= 0x2A9) continue;
            s32 effectId = *(s32*)((u8*)AbilityEffect(gGameDb, ability) + 0xC);
            s32 limbSlot = *(s32*)((u8*)AbilityEffect(gGameDb, ability) + 0x8);
            u32 emitter = EffectEmitter(gGameDb, effectId, limbSlot);
            SysSprintf(lbl_803C5358, lbl_80358C68 + 0x70);
            u8* fx = ((u8**)((u8**)(gBattle + 0xA00))[i])[limbSlot];
            if (fx != NULL) {
                u8* target = *(u8**)(fx + 0x20);
                if (target != NULL) target -= 0x264;
                PlayAttachedParticleEffectAtEmitter(target, kAbilityEffects[effect], emitter);
            }
        }
    }
    *(f32*)(arg0 + 0xC) = lbl_805FD228;
    SysSprintf(lbl_803C5358, lbl_805F8698);
}

// 0x80050164
// Resets the widget record chain through the per-type dispatch table: bumps the entry counter
// (id 0x3C) and clears/initializes the fields of the new entry, unless the last entry already is
// the terminator pattern (7,0,6,7 -> nothing to do; 7,7,7,0 -> keep the counter).
// `arg0` is unused; `ctx` holds the object handle (+8) and its dispatch type (+0xC).
void fn_80050164(u8* arg0, u8* ctx) {
    void* obj = *(void**)(ctx + 8);
    u8 type = *(u8*)(ctx + 0xC);
    s32 n = (*(OverlayGetFn*)(lbl_80348F38 + type * 4 + 0x30))(obj, 0x3C);
    if (n == 0) return;
    s32 base = (n - 1) * 0x1A;
    s32 a = (*(OverlayGetFn*)(lbl_80348F38 + *(u8*)(ctx + 0xC) * 4 + 0x30))(*(void**)(ctx + 8), base + 0x16B);
    s32 b = (*(OverlayGetFn*)(lbl_80348F38 + *(u8*)(ctx + 0xC) * 4 + 0x30))(*(void**)(ctx + 8), base + 0x16C);
    s32 c = (*(OverlayGetFn*)(lbl_80348F38 + *(u8*)(ctx + 0xC) * 4 + 0x30))(*(void**)(ctx + 8), base + 0x16D);
    s32 d = (*(OverlayGetFn*)(lbl_80348F38 + *(u8*)(ctx + 0xC) * 4 + 0x30))(*(void**)(ctx + 8), base + 0x16E);
#define SETF(id, v) (*(OverlaySetFn*)(lbl_80348F38 + *(u8*)(ctx + 0xC) * 4 + 0x24))(*(void**)(ctx + 8), (id), (v))
    if (a == 7 && b == 0 && c == 6 && d == 7) return;
    if (!(a == 7 && b == 7 && c == 7 && d == 0)) {
        SETF(0x3C, n + 1);
        base = n * 0x1A;
        SETF(base + 0x158, 0);
        SETF(base + 0x160, 0);
        SETF(base + 0x161, 0);
        SETF(base + 0x162, 0xF);
        SETF(base + 0x163, 0xF);
        SETF(base + 0x164, 0xF);
        SETF(base + 0x165, 0);
    }
    SETF(base + 0x15C, 0x16);
    SETF(base + 0x169, 0);
    SETF(base + 0x16A, 0);
    SETF(base + 0x16B, 7);
    SETF(base + 0x16C, 0);
    SETF(base + 0x16D, 6);
    SETF(base + 0x16E, 7);
    SETF(0x13E, 1);
    SETF(0x13F, 0);
    SETF(0x140, 0);
    SETF(0x141, 0);
    SETF(0x142, F32Bits(lbl_805FD274));
#undef SETF
}

}  // extern "C"
