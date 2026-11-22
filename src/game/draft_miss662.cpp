// Player/enemy pathing and hit handling in 0x8005xxxx-0x8006xxxx: waypoint walkers
// (fn_8005E694, fn_8005E96C, fn_8005EED4), the damage entry point (fn_8006A2D0), the
// enter/exit trigger dispatch of a zone object (fn_8006DA84) and a random-beam endpoint
// generator (fn_8006EF78).
#include <math.h>
#include <string.h>
#include "types.h"

extern "C++" void* gEventMgr;           // 0x805FB6C8
extern "C++" s32   gZoneResourceSize;   // 0x805FB94C
extern "C++" f32   GetOne(void);        // 0x80014208 (session.cpp defines it with C++ linkage)

extern "C" {
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
s32  PlaySoundAt(void* sound, int a, const void* pos, int b);                                 // 0x80209ABC
u32  RandomU32(void);                                                                         // 0x80143460
s32  TriggerDoAction(void* action, void* a1, void* a2);                                       // 0x801E0054
void fn_800602E8(u8* arg0, u8* color, f32 farg0);                                             // 0x800602E8
void* fn_80060A28(u8* p);                                                                     // 0x80060A28
void fn_801B62D8(u8* arg0, u8* arg1, s32 arg2, s32 arg3, s32 arg4, f32 farg0);                // 0x801B62D8
BOOL fn_801C6634(u8* col);                                                                    // 0x801C6634
u8*  fn_801C668C(u8* key);                                                                    // 0x801C668C
void fn_801C6724(u8* out, u8* obj, s32 zone);                                                 // 0x801C6724
void fn_801C6838(u8* out, u8* a, u8* b);                                                      // 0x801C6838
u8*  fn_801C6BCC(u8* key);                                                                    // 0x801C6BCC
void fn_801C6C80(u8* out, u8* rec, u8* color, u8* obj);                                       // 0x801C6C80 (4th arg: object pointer in these callers)
s32  fn_801C71EC(u8* key);                                                                    // 0x801C71EC
void fn_801ED34C(u8* arg0, s32 arg1);                                                         // 0x801ED34C
f64  fn_8033F1F4(f64 x);                                                                      // 0x8033F1F4 (MSL acos)
s32  rand(void);                                                                              // 0x80337150

extern u8  gFlag805FB948;       // 0x805FB948
extern u8  gWorldState[];       // 0x803E6D00 (first word: world block pointer)
extern f32 lbl_8035A080[];      // 0x8035A080 (4 floats)
extern u8  lbl_803C54F0[];      // 0x803C54F0 (16 records of 0x18 bytes)
extern u32 lbl_805FB854;        // 0x805FB854 (32-bit frame counter)
extern u32 lbl_805FB9A4;        // 0x805FB9A4 (lock flag)
extern f32 lbl_805FD3F0;        // 0x805FD3F0 (0.0f)
extern f32 lbl_805FD3FC;        // 0x805FD3FC (1.0f)
extern f32 lbl_805FD404;        // 0x805FD404 (-1.0f)
extern f32 lbl_805FD420;        // 0x805FD420 (2/pi)
extern f32 lbl_805FD4E8;        // 0x805FD4E8 (0.0f)
extern f32 lbl_805FD598;        // 0x805FD598 (0.0f)
extern f32 lbl_805FD59C;        // 0x805FD59C (80.0f)
extern f32 lbl_805FD5A0;        // 0x805FD5A0 (0.0001f)
extern f32 lbl_805FD5A4;        // 0x805FD5A4 (-0.0001f)
extern f32 lbl_805FD5A8;        // 0x805FD5A8 (1.0f)
extern f32 lbl_805FD5B0;        // 0x805FD5B0 (2^-32)
extern f32 lbl_805FFD28;        // 0x805FFD28 (minimum squared reach)
}

static const u32 kHashEnter = 0x4E2402DEu;  // "Enter"
static const u32 kHashExit  = 0xA29C69BAu;  // "Exit"

// A 4-byte waypoint key: u8, u8, u16 (copied as a unit).
static void Key4(u8* dst, const u8* src) {
    memcpy(dst, src, 4);
}

// The key at arg1+8 (the chosen waypoint) is reachable from the key at arg0+0xBA0 (the current
// one): either they match in the first two bytes or fn_801C6838 yields a valid link.
static BOOL KeyReachable(u8* arg0, u8* arg1) {
    u8 cur[4];
    u8 dst[4];
    u8 link[4];
    u8 b0 = *(u8*)(arg1 + 0x8);
    u8 b1 = *(u8*)(arg1 + 0x9);
    if (b0 == *(u8*)(arg0 + 0xBA0) && b1 == *(u8*)(arg0 + 0xBA1)) {
        return 1;
    }
    Key4(dst, arg1 + 0x8);
    Key4(cur, arg0 + 0xBA0);
    fn_801C6838(link, cur, dst);
    return fn_801C6634(link);
}

extern "C" {

// 0x8005E694
// Picks the waypoint the walker heads for: takes the object at arg1+0x14 (or looks one up with
// fn_80060A28), validates it, then chooses the target key into arg1+8 and the aim point into
// arg0+0xB88. Returns 1 when a target was set, 0 otherwise.
s32 fn_8005E694(u8* arg0, u8* arg1) {
    u8 tmp[4];

    *(s32*)(arg0 + 0xA08) = *(s32*)(arg0 + 0xA08) & ~8;
    u8* o = *(u8**)(arg1 + 0x14);
    if (o != NULL && (*(s32(**)(u8*))(*(u8**)o + 0x4C))(o) == 0 &&
        *(s32*)(arg1 + 0x0) != (s32)0x4553808C) {
        *(void**)(arg1 + 0x10) = *(void**)(arg1 + 0x14);
    } else {
        *(void**)(arg1 + 0x10) = fn_80060A28(arg0);
    }
    u8* t = *(u8**)(arg1 + 0x10);
    if (t == NULL || (*(s32(**)(u8*))(*(u8**)t + 0x4C))(t) != 0) {
        return 0;
    }

    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + *(u16*)(*(u8**)(arg0 + 0x4) + 0x30) * gZoneResourceSize;
    s32 skip = 0;
    if (*(u8*)(zone + 0xC0) == 0 || (*(s32*)(arg0 + 0xA08) & 1) || (*(s32*)(arg0 + 0xA08) & 0x20)) {
        skip = 1;
    }
    if (skip == 0) {
        u8* r = *(u8**)(arg1 + 0x10);
        if (*(s32*)(*(u8**)(r + 0x4) + 0x1C) & 0x20) {
            bool useA = false;
            if (fn_801C6634(r + 0xBA4)) {
                if (*(s32*)(arg0 + 0xA08) & 2) {
                    useA = true;
                } else if (*(u8*)(r + 0xBA0) == *(u8*)(arg0 + 0xBA4) &&
                           *(u8*)(r + 0xBA1) == *(u8*)(arg0 + 0xBA5)) {
                    useA = true;
                }
            }
            if (useA) {
                *(u8*)(arg1 + 0x8) = *(u8*)(r + 0xBA4);
                *(u8*)(arg1 + 0x9) = *(u8*)(r + 0xBA5);
                *(u16*)(arg1 + 0xA) = *(u16*)(r + 0xBA6);
            } else if (fn_801C6634(r + 0xBA0)) {
                *(u8*)(arg1 + 0x8) = *(u8*)(r + 0xBA0);
                *(u8*)(arg1 + 0x9) = *(u8*)(r + 0xBA1);
                *(u16*)(arg1 + 0xA) = *(u16*)(r + 0xBA2);
            } else {
                return 0;
            }
        } else {
            fn_801C6724(tmp, r + 0x18, *(u16*)(*(u8**)(r + 0x4) + 0x30));
            *(u8*)(arg1 + 0x8) = tmp[0];
            *(u8*)(arg1 + 0x9) = tmp[1];
            *(u16*)(arg1 + 0xA) = *(u16*)(tmp + 2);
        }
        if (!(*(s32*)(arg0 + 0xA08) & 1)) {
            *(s32*)(arg0 + 0xA08) = *(s32*)(arg0 + 0xA08) & ~2;
        }
        if (!KeyReachable(arg0, arg1)) {
            *(s32*)(arg0 + 0xA08) = *(s32*)(arg0 + 0xA08) & ~8;
            return 0;
        }
    }

    u8* r = *(u8**)(arg1 + 0x10);
    f32* aim = (*(f32*(**)(u8*, u8*, u8*))(*(u8**)r + 0x44))(r, arg0 + 0x18, arg0);
    *(f32*)(arg0 + 0xB88) = aim[0];
    *(f32*)(arg0 + 0xB8C) = aim[1];
    *(f32*)(arg0 + 0xB90) = aim[2];
    *(s32*)(arg0 + 0xA08) = *(s32*)(arg0 + 0xA08) | 8;
    return 1;
}

// 0x8005E96C
// Walker steering towards a fixed point (record of lbl_803C54F0 or the object at arg1+0x10):
// counts down the timer at arg1+0x18, then (zone with waypoint data) picks the neighbouring
// waypoint whose direction best matches the heading to the target, or (no data) aims straight
// past the target. Returns 1 when a heading was set, 0 when it gave up.
s32 fn_8005E96C(u8* arg0, u8* arg1) {
    f32 tx, ty, tz;
    u8* e = lbl_803C54F0 + *(u8*)(arg0 + 0xB86) * 0x18;
    if (*(f32*)(e + 0x10) >= *(f32*)(arg1 + 0x1C)) {
        tx = *(f32*)(e + 0x0);
        ty = *(f32*)(e + 0x4);
        tz = *(f32*)(e + 0x8);
    } else {
        u8* o = *(u8**)(arg1 + 0x10);
        if (o == NULL) return 0;
        tx = *(f32*)(o + 0x18);
        ty = *(f32*)(o + 0x1C);
        tz = *(f32*)(o + 0x20);
    }

    f32 remain = *(f32*)(arg1 + 0x18) - *(f32*)(arg0 + 0x24);
    *(f32*)(arg1 + 0x18) = remain;
    if (remain <= lbl_805FD3F0) {
        EventSend(gEventMgr, arg0, arg0, 0x9C44ADFDu, 0, 0, 1, 0);
        return 0;
    }

    u8* pos = *(u8**)(arg0 + 0x4);
    u8* zone = *(u8**)(*(u8**)gWorldState + 0x1A4) + *(u16*)(pos + 0x30) * gZoneResourceSize;
    if (*(u8*)(zone + 0xC0) == 0) {
        // No waypoint data: aim at the mirror image of the target about the position.
        f32 px = *(f32*)(pos + 0x0);
        f32 py = *(f32*)(pos + 0x4);
        f32 pz = *(f32*)(pos + 0x8);
        f32 dx = px - tx;
        f32 dy = py - ty;
        f32 dz = pz - tz;
        f32 ax = (px + dx) + lbl_805FD3F0;
        f32 ay = (py + dy) + lbl_805FD3FC;
        f32 az = (pz + dz) + lbl_805FD3F0;
        s32 flags = *(s32*)(arg0 + 0xA08);
        *(f32*)(arg0 + 0xB88) = ax;
        *(f32*)(arg0 + 0xB8C) = ay;
        *(f32*)(arg0 + 0xB90) = az;
        *(s32*)(arg0 + 0xA08) = flags | 8;
        return 1;
    }

    s32 search = 0;
    if (!fn_801C6634(arg1 + 0x8) || (*(s32*)(arg0 + 0xA08) & 2)) {
        search = 1;
    } else {
        s32 f838 = *(s32*)(arg0 + 0x838);
        if ((f838 & 8) || (f838 & 0x10)) search = 1;
    }
    if (search) {
        u8* cur = fn_801C668C(arg0 + 0xBA0);
        if (cur != NULL && *(u8*)(cur + 0x14) != 0) {
            u8* p0 = *(u8**)(arg0 + 0x4);
            f32 cx = tx - *(f32*)(p0 + 0x0);
            f32 cy = ty - *(f32*)(p0 + 0x4);
            f32 cz = tz - *(f32*)(p0 + 0x8);
            f32 best = lbl_805FD3F0;
            f32 cl2 = (cz * cz) + ((cx * cx) + (cy * cy));
            f32 clen = (cl2 <= lbl_805FD3F0) ? lbl_805FD3F0 : (f32)sqrt((f64)cl2);   // original: frsqrte + 2 Newton steps
            u8* link = cur;
            u8* key = cur + 0x16;
            for (s32 i = 0; i < (s32)*(u8*)(cur + 0x14); i++) {
                u8* n = fn_801C668C(key);
                u8* p1 = *(u8**)(arg0 + 0x4);
                f32 dx = *(f32*)(n + 0x4) - *(f32*)(p1 + 0x0);
                f32 dy = *(f32*)(n + 0x8) - *(f32*)(p1 + 0x4);
                f32 dz = *(f32*)(n + 0xC) - *(f32*)(p1 + 0x8);
                f32 l2 = (dz * dz) + ((dx * dx) + (dy * dy));
                f32 dot = (dz * cz) + ((dx * cx) + (dy * cy));
                f32 len = (l2 <= lbl_805FD3F0) ? lbl_805FD3F0 : (f32)sqrt((f64)l2);
                f32 prod = len * clen;
                f32 ang;
                if (prod <= lbl_805FD3F0) {
                    ang = lbl_805FD3F0;
                } else {
                    f32 c = dot / prod;
                    if (c < lbl_805FD404) c = lbl_805FD404;
                    else if (c > lbl_805FD3FC) c = lbl_805FD3FC;
                    ang = lbl_805FD420 * (f32)fn_8033F1F4((f64)c);
                }
                if (ang > best) {
                    best = ang;
                    *(u8*)(arg1 + 0x8) = *(u8*)(link + 0x16);
                    *(u8*)(arg1 + 0x9) = *(u8*)(link + 0x17);
                    *(u16*)(arg1 + 0xA) = *(u16*)(link + 0x18);
                    u8* c0 = fn_801C668C(arg0 + 0xBA0);
                    if (c0 != NULL) {
                        f32 x = *(f32*)(c0 + 0x4);
                        f32 y = *(f32*)(c0 + 0x8);
                        f32 z = *(f32*)(c0 + 0xC);
                        *(f32*)(arg0 + 0xB94) = x;
                        *(f32*)(arg0 + 0xB98) = y;
                        *(f32*)(arg0 + 0xB9C) = z;
                        *(f32*)(arg0 + 0x9E8) = x;
                        *(f32*)(arg0 + 0x9EC) = y;
                        *(f32*)(arg0 + 0x9F0) = z;
                    }
                    *(s32*)(arg0 + 0xA08) = (*(s32*)(arg0 + 0xA08) | 0x10) & ~2;
                }
                key += 4;
                link += 4;
            }
        }
        if (!fn_801C6634(arg1 + 0x8)) return 0;
    }
    return 1;
}

// 0x8005EED4
// Steps the walker along its current waypoint record (key at arg1+0xC): sets the aim point
// (arg0+0xB88) and the flag bits, and when within reach of the record fires its trigger
// (record+0x40), advances to the next key and re-validates the chosen waypoint (arg1+8).
// Returns 1 on success, 0 when the key has no record or the next waypoint is unreachable.
s32 fn_8005EED4(u8* arg0, u8* arg1) {
    u8 key[4];
    u8 out[4];
    u8 col[4];
    u8 tmp[4];

    Key4(key, arg1 + 0xC);
    u8* rec = fn_801C6BCC(key);
    if (rec == NULL) return 0;

    u8* host = *(u8**)(arg0 + 0xA14);
    if (*(u8*)(arg0 + 0xB84) == 1 && host != NULL) {
        f32* aim = (*(f32*(**)(u8*, u8*, u8*))(*(u8**)host + 0x44))(host, arg0 + 0x18, arg0);
        *(f32*)(arg0 + 0xB88) = aim[0];
        *(f32*)(arg0 + 0xB8C) = aim[1];
        *(f32*)(arg0 + 0xB90) = aim[2];
    } else {
        *(f32*)(arg0 + 0xB88) = *(f32*)(rec + 0x18);
        *(f32*)(arg0 + 0xB8C) = *(f32*)(rec + 0x1C);
        *(f32*)(arg0 + 0xB90) = *(f32*)(rec + 0x20);
    }
    s32 flags = *(s32*)(arg0 + 0xA08) | 8;
    *(s32*)(arg0 + 0xA08) = flags;
    if (*(s32*)(rec + 0x14) & 1) {
        *(s32*)(arg0 + 0xA08) = flags | 4;
    } else {
        *(s32*)(arg0 + 0xA08) = flags & ~4;
    }

    u8* pos = *(u8**)(arg0 + 0x4);
    f32 reach = *(f32*)(rec + 0x30) * *(f32*)(rec + 0x30);
    f32 dx = *(f32*)(rec + 0x18) - *(f32*)(pos + 0x0);
    f32 dy = *(f32*)(rec + 0x1C) - *(f32*)(pos + 0x4);
    f32 dz = *(f32*)(rec + 0x20) - *(f32*)(pos + 0x8);
    if (reach < lbl_805FFD28) reach = lbl_805FFD28;
    if (!((dz * dz) + ((dx * dx) + (dy * dy)) >= reach)) {
        // Within reach: pick the next key from arg0's key at +0xBD4 and swap it with arg1's key.
        u16 savedH = *(u16*)(arg1 + 0xE);
        u8  savedB = *(u8*)(arg1 + 0xD);
        u8  savedA = *(u8*)(arg1 + 0xC);
        *(u16*)(col + 2) = *(u16*)(arg0 + 0xBD6);
        col[1] = *(u8*)(arg0 + 0xBD5);
        col[0] = *(u8*)(arg0 + 0xBD4);
        fn_801C6C80(out, rec, col, arg0);
        *(u16*)(arg1 + 0xE) = *(u16*)(out + 2);
        *(u8*)(arg1 + 0xD) = out[1];
        *(u8*)(arg1 + 0xC) = out[0];
        *(u16*)(arg0 + 0xBD6) = savedH;
        *(u8*)(arg0 + 0xBD5) = savedB;
        *(u8*)(arg0 + 0xBD4) = savedA;
        TriggerDoAction(rec + 0x40, arg0, NULL);
        *(u8*)(arg1 + 0x8) = 0xFF;
        *(s32*)(arg0 + 0xA08) = *(s32*)(arg0 + 0xA08) & ~2;
        Key4(key, arg1 + 0xC);
        rec = fn_801C6BCC(key);
        if (rec == NULL) return 0;
    }

    if (!(*(s32*)(arg0 + 0xA08) & 4) && !fn_801C6634(arg1 + 0x8)) {
        *(s32*)(arg0 + 0xA08) = *(s32*)(arg0 + 0xA08) & ~2;
        if (fn_801C6634(rec + 0x3C)) {
            *(u8*)(arg1 + 0x8) = *(u8*)(rec + 0x3C);
            *(u8*)(arg1 + 0x9) = *(u8*)(rec + 0x3D);
            *(u16*)(arg1 + 0xA) = *(u16*)(rec + 0x3E);
        } else {
            fn_801C6724(tmp, rec + 0x18, *(u16*)(*(u8**)(arg0 + 0x4) + 0x30));
            *(u8*)(arg1 + 0x8) = tmp[0];
            *(u8*)(arg1 + 0x9) = tmp[1];
            *(u16*)(arg1 + 0xA) = *(u16*)(tmp + 2);
        }
        if (!KeyReachable(arg0, arg1)) return 0;
    }
    return 1;
}

// 0x8006A2D0
// Damage entry point: applies amount farg0 (arg2 = damage kind) from attacker arg1 to arg0.
// Runs the hit triggers, decides whether the hit counts (armour/immunity), subtracts the amount
// from the health at +4->+0x24, records who hit us (+0x198), fires the hit/sound triggers and
// sets the hit-flash colour on the object at +0x90.
void fn_8006A2D0(u8* arg0, u8* arg1, s32 arg2, s32 arg3, s32 arg4, f32 farg0) {
    f32 dmg = farg0;
    u8* player;

    if (arg1 != NULL && (*(s32*)(*(u8**)(arg1 + 0x4) + 0x1C) & 0x80)) {
        if (arg1 == arg0) return;
        if (*(u8**)(arg1 + 0x23C) == arg0) return;
    }

    if (arg1 != NULL && ((*(s32*)(arg0 + 0x15C) & 0x20) || *(s32*)(arg0 + 0x18C) != 0) &&
        (gFlag805FB948 == 0 || lbl_805FB9A4 == 0)) {
        dmg *= GetOne();   // the asm passes (arg1, arg0); GetOne ignores them
    }
    fn_801B62D8(arg0, arg1, arg2, arg3, arg4, dmg);

    s32 ok1 = 1;
    s32 ok2 = 1;
    *(s32*)(arg0 + 0xC0) = arg2;
    if ((*(u32*)(arg0 + 0x38) >> 27) & 0xF) {
        ok1 = TriggerDoAction(arg0 + 0x38, arg0, arg1);
    }
    if (ok1 == 1) {
        u8* a = *(u8**)(arg0 + 0x9C);
        if (a != NULL && ((*(u32*)(a + 0x68) >> 27) & 0xF)) {
            ok2 = TriggerDoAction(a + 0x68, arg0, arg1);
        }
    }

    player = *(u8**)(*(u8**)gWorldState + 0x8);
    s32 counts;
    if (!(ok1 == 1 && ok2 == 1)) {
        counts = 0;
    } else if (*(s32*)(*(u8**)(arg0 + 0x4) + 0x20) & 0x20) {
        counts = 0;
    } else if ((*(s32*)(arg0 + 0x15C) & 0x801E) && arg1 != NULL && arg1 != player &&
               (!(*(s32*)(*(u8**)(arg1 + 0x4) + 0x1C) & 0x80) ||
                *(s32*)(*(u8**)(arg1 + 0x228) + 0xB4) == 0)) {
        counts = 0;
    } else {
        counts = 1;
    }
    if (counts == 0) dmg = lbl_805FD4E8;

    if (*(s32*)(arg0 + 0x15C) & 0x100) {
        s32* hook = *(s32**)(arg0 + 0xA4);
        if (hook != NULL && *hook != 0) {
            u8* st = *(u8**)(arg0 + 0x4);
            s32 bits;
            memcpy(&bits, &dmg, 4);
            *(s32*)(st + 0x2C) = *(s32*)(st + 0x2C) | 0x10;
            EventSend(gEventMgr, arg0, arg0, 0x39E30270u, arg2, bits, 1, 0);
        }
    }

    if (counts == 0) return;
    if (dmg == lbl_805FD4E8) return;

    *(s32*)(arg0 + 0xC0) = arg2;
    *(void**)(arg0 + 0x198) = NULL;
    if (arg1 != NULL &&
        (arg1 == player || ((*(s32*)(*(u8**)(arg1 + 0x4) + 0x1C) & 0x80) &&
                            *(s32*)(*(u8**)(arg1 + 0x228) + 0xB4) != 0))) {
        *(u8**)(arg0 + 0x198) = player;
    }
    if (arg1 != NULL && (*(s32*)(*(u8**)(arg1 + 0x4) + 0x1C) & 0x40)) {
        u8* by = *(u8**)(arg1 + 0x198);
        if (by != NULL &&
            (by == player || ((*(s32*)(*(u8**)(by + 0x4) + 0x1C) & 0x80) &&
                              *(s32*)(*(u8**)(by + 0x228) + 0xB4) != 0))) {
            *(void**)(arg0 + 0x198) = *(void**)(arg1 + 0x198);
        }
    }
    if (*(s32*)(arg0 + 0x15C) & 0x801E) {
        *(u8**)(arg0 + 0x198) = player;
    }

    u8* st = *(u8**)(arg0 + 0x4);
    *(f32*)(st + 0x24) = *(f32*)(st + 0x24) - dmg;

    u32 frame = lbl_805FB854;
    if (frame - *(u32*)(arg0 + 0xC4) > 3) {
        u8* a = *(u8**)(arg0 + 0x9C);
        if (a != NULL && ((*(u32*)(a + 0x6C) >> 27) & 0xF)) {
            TriggerDoAction(a + 0x6C, arg0, arg1);
        }
        if ((*(u32*)(arg0 + 0x3C) >> 27) & 0xF) {
            TriggerDoAction(arg0 + 0x3C, arg0, arg1);
        }
    }
    u8* a2 = *(u8**)(arg0 + 0x9C);
    *(u32*)(arg0 + 0xC4) = frame;
    if (a2 != NULL && ((*(u32*)(a2 + 0x70) >> 27) & 0xF)) {
        TriggerDoAction(a2 + 0x70, arg0, arg1);
    }
    if ((*(u32*)(arg0 + 0x40) >> 27) & 0xF) {
        TriggerDoAction(arg0 + 0x40, arg0, arg1);
    }
    PlaySoundAt(*(void**)(*(u8**)(arg0 + 0x9C) + 0x2C), 0, *(u8**)(arg0 + 0x4), 0);

    u8* flash = *(u8**)(arg0 + 0x90);
    if (flash != NULL && arg2 != 9 && arg2 != 4) {
        *(f32*)(flash + 0x1C) = lbl_8035A080[0];
        *(f32*)(flash + 0x20) = lbl_8035A080[1];
        *(f32*)(flash + 0x24) = lbl_8035A080[2];
        *(f32*)(flash + 0x28) = lbl_8035A080[3];
    }
}

// 0x8006DA84
// Enter handler of a trigger zone: fires the "enter" action slots (on the object's +0x9C
// definition and on itself) with arg1 as the activator, posts the "Enter" event, and, if the
// object has pending sub-entries (+0x1D4, 5 slots of 8 bytes) and no disable counter (+0x208),
// fires the "exit" actions for each of them. Finally, for flagged objects with a valid record
// key (+0x1FC), picks a follow-up key and hands the colour to fn_800602E8 on arg1.
void fn_8006DA84(u8* arg0, u8* arg1) {
    u8* def = *(u8**)(arg0 + 0x9C);
    if (def != NULL && ((*(u32*)(def + 0x9C) >> 27) & 0xF)) {
        TriggerDoAction(def + 0x9C, arg1, arg0);
    }
    if ((*(u32*)(arg0 + 0x6C) >> 27) & 0xF) {
        TriggerDoAction(arg0 + 0x6C, arg1, arg0);
    }
    EventSend(gEventMgr, arg0, arg0, kHashEnter, 0, 0, 1, 0);

    if (*(s32*)(arg0 + 0x208) == 0) {
        f32 t = *(f32*)(arg0 + 0x1BC);
        if (t > lbl_805FD598) {
            *(f32*)(arg0 + 0x1C0) = t;
            u8* slot = arg0;
            for (u32 i = 0; i < 5; i++) {
                u8* who = *(u8**)(slot + 0x1D4);
                if (who != NULL) {
                    *(void**)(slot + 0x1D4) = NULL;
                    u8* d = *(u8**)(arg0 + 0x9C);
                    if (d != NULL && ((*(u32*)(d + 0xA4) >> 27) & 0xF)) {
                        TriggerDoAction(d + 0xA4, who, arg0);
                    }
                    if ((*(u32*)(arg0 + 0x74) >> 27) & 0xF) {
                        TriggerDoAction(arg0 + 0x74, who, arg0);
                    }
                    EventSend(gEventMgr, arg0, arg0, kHashExit, 0, 0, 1, 0);
                }
                slot += 8;
            }
        }
    }

    if ((*(s32*)(arg0 + 0x1B0) & 0x10) && (*(s32*)(*(u8**)(arg1 + 0x4) + 0x1C) & 0x20) &&
        fn_801C71EC(arg0 + 0x1FC) != 0) {
        u8 key[4];
        Key4(key, arg0 + 0x1FC);
        u8* rec = fn_801C6BCC(key);
        if (rec != NULL) {
            u8 pick[4];
            u8 out[4];
            u8 sel[4];
            u8 color[4];
            sel[0] = 0;
            sel[1] = 0;
            *(u16*)(sel + 2) = 0xFFFF;
            fn_801C6C80(out, rec, sel, arg1);
            if (fn_801C71EC(out) != 0) {
                pick[0] = 0;
                pick[1] = 0;
                *(u16*)(pick + 2) = 0xFFFF;
                fn_801ED34C(pick, out[0]);
                pick[1] = out[1];
                *(u16*)(pick + 2) = *(u16*)(out + 2);
                Key4(color, pick);
                fn_800602E8(arg1, color, lbl_805FD59C);
            }
        }
    }
}

// One random sample in [lo, hi): lo + (hi - lo) * (RandomU32() / 2^32).
static f32 RandRange(f32 lo, f32 hi) {
    f32 r = lbl_805FD5B0 * (f32)RandomU32();
    return lo + (hi - lo) * r;
}

// Moves the point pt by a random vector in the unit cube, made perpendicular to the direction
// stored at obj+0x1A0 (the random vector minus its projection onto that direction), scaled by t.
static void RandomPerpOffset(u8* obj, f32* pt, f32 t) {
    f32 one = lbl_805FD5A8;
    f32 lo = -one;
    f32 v0 = RandRange(lo, one);
    f32 v1 = RandRange(lo, one);
    f32 v2 = RandRange(lo, one);
    f32 dx = *(f32*)(obj + 0x1A0);
    f32 dy = *(f32*)(obj + 0x1A4);
    f32 dz = *(f32*)(obj + 0x1A8);
    f32 l2 = (dz * dz) + ((dx * dx) + (dy * dy));
    f32 p0, p1, p2;
    if (l2 <= lbl_805FD5A0 && l2 >= lbl_805FD5A4) {
        p0 = lbl_805FD598;
        p1 = lbl_805FD598;
        p2 = lbl_805FD598;
    } else {
        f32 dot = (dz * v2) + ((dx * v0) + (dy * v1));
        p0 = (dx * dot) / l2;
        p1 = (dy * dot) / l2;
        p2 = (dz * dot) / l2;
    }
    f32 w0 = v0 - p0;
    f32 w1 = v1 - p1;
    f32 w2 = v2 - p2;
    pt[0] = pt[0] + w0 * t;
    pt[1] = pt[1] + w1 * t;
    pt[2] = pt[2] + w2 * t;
}

// 0x8006EF78
// Builds a jittered line segment: start = arg0+0x194, end = start + dir(+0x1A0) * length(+0x160);
// each endpoint is nudged by a random perpendicular offset (scale +0x164), then the two points
// are written to the first and last 3-float slot of the point array at arg1+0x4C (count at
// arg1+0x38), in random order.
void fn_8006EF78(u8* arg0, u8* arg1) {
    f32 len = *(f32*)(arg0 + 0x160);
    f32 t = *(f32*)(arg0 + 0x164);
    f32 a[3];
    f32 b[3];
    a[0] = *(f32*)(arg0 + 0x194);
    a[1] = *(f32*)(arg0 + 0x198);
    a[2] = *(f32*)(arg0 + 0x19C);
    b[0] = a[0] + *(f32*)(arg0 + 0x1A0) * len;
    b[1] = a[1] + *(f32*)(arg0 + 0x1A4) * len;
    b[2] = a[2] + *(f32*)(arg0 + 0x1A8) * len;

    RandomPerpOffset(arg0, a, t);
    RandomPerpOffset(arg0, b, t);

    u8* last = arg1 + (*(s32*)(arg1 + 0x38) - 1) * 0xC;
    const f32* first = (rand() % 2 != 0) ? a : b;
    const f32* second = (first == a) ? b : a;
    *(f32*)(arg1 + 0x4C) = first[0];
    *(f32*)(arg1 + 0x50) = first[1];
    *(f32*)(arg1 + 0x54) = first[2];
    *(f32*)(last + 0x4C) = second[0];
    *(f32*)(last + 0x50) = second[1];
    *(f32*)(last + 0x54) = second[2];
}

}  // extern "C"
