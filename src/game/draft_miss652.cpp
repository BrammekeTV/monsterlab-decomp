#include "types.h"
#include "engine/math.h"

// Globals defined in C++ files (C++ linkage).
extern "C++" void* gEventMgr;    // 0x805FB6C8 (game_ui.cpp)
extern "C++" s32 gScreenWidth;   // 0x805F9388 (catch_minigame.cpp)
extern "C++" s32 gScreenHeight;

extern "C" {
// Callees.
void CameraSetMatrix(void* camera, const f32 mtx[3][4]);                // 0x801D6814
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
BOOL PadButtonHeld(s32 button);                                         // 0x801474EC
u32  RandomU32(void);                                                   // 0x80143460
s32  RaySceneTest(void* ray);                                           // 0x80070458 (ignores its argument)
s32  TriggerDoAction(void* action, void* a1, void* a2);                 // 0x801E0054
void fn_80009840(u8* p);                                                // 0x80009840
void fn_80053C2C(u8* c, f32 dt);                                        // 0x80053C2C
void fn_8005E1B8(u8* p);                                                // 0x8005E1B8
s32  fn_8005E514(u8* p, u8* state);                                     // 0x8005E514
s32  fn_8005E694(u8* p, u8* state);                                     // 0x8005E694
s32  fn_8005E96C(u8* p, u8* state);                                     // 0x8005E96C
s32  fn_8005EED4(u8* p, u8* state);                                     // 0x8005EED4
void fn_8005FD4C(u8* p, u8* entry);                                     // 0x8005FD4C
void* fn_80060A28(u8* p);                                               // 0x80060A28
s32  fn_8006D9A0(u8* p);                                                // 0x8006D9A0
void fn_8006DA84(u8* p, u8* target);                                    // 0x8006DA84
void fn_80070FAC(u8* p, f32 v);                                         // 0x80070FAC
void fn_800712B4(void);                                                 // 0x800712B4
void fn_80073A7C(void* camData, const f32* pos3, f32* outX, f32* outY); // 0x80073A7C
s32  fn_8008928C(u8* p, u8** out);                                      // 0x8008928C
void fn_80089B30(u8* p);                                                // 0x80089B30
s32  fn_8008C0B4(u8* cam);                                              // 0x8008C0B4 (returns the popped list entry)
s32  fn_8008C148(u8* cam, f32 x, f32 y);                                // 0x8008C148
s32  fn_8008C1BC(u8* cam, s32 x, s32 y);                                // 0x8008C1BC
s32  fn_8008C2EC(u8* cam);                                              // 0x8008C2EC
s32  fn_8008C380(u8* cam);                                              // 0x8008C380
void fn_8008C414(u8* cam, u8* target);                                  // 0x8008C414 (stores a pointer at +0x378)
void fn_8008D670(u8* p);                                               // 0x8008D670
s32  fn_8008D67C(u8* p);                                                // 0x8008D67C
void fn_801C4B90(u8* obj, f32* a, f32* b, s32 c, s32 d, u8* e);         // 0x801C4B90
void fn_801C5CC0(u8* obj, f32* a, f32* b);                              // 0x801C5CC0

// Data.
extern s32 lbl_803468E8[];   // 0x803468E8 (weight tables)
extern s32 lbl_803468F8[];   // 0x803468F8
extern s32 lbl_80346908[];   // 0x80346908
extern s32 lbl_80346918[];   // 0x80346918
extern s32 lbl_80346928[];   // 0x80346928
extern s32 lbl_80346938[];   // 0x80346938
extern s32 lbl_80346948[];   // 0x80346948
extern f32 lbl_803469A8[];   // 0x803469A8
extern u8  lbl_803C4298[];   // 0x803C4298
extern u8  lbl_8047C640[];   // 0x8047C640 (camera)
extern void* lbl_805FB5F8;   // 0x805FB5F8 (current camera)
extern f32 lbl_805FD3F0;     // 0x805FD3F0 (0.0f)
extern f32 lbl_805FD404;     // 0x805FD404 (-1.0f)
extern f32 lbl_805FD468;     // 0x805FD468 (0.0f)
extern f32 lbl_805FD480;     // 0x805FD480 (0.5f)
extern f32 lbl_805FD484;     // 0x805FD484 (1.0f)
extern f32 lbl_805FD598;     // 0x805FD598 (0.0f)
extern f32 lbl_805FD928;     // 0x805FD928 (0.0f)
extern f32 lbl_805FD940;     // 0x805FD940 (0.5f)
extern f32 lbl_805FD944;     // 0x805FD944 (100.0f)
extern f32 lbl_805FD95C;     // 0x805FD95C (-7.0f)
extern f32 lbl_805FD9B4;     // 0x805FD9B4 (1.0f)
extern f32 lbl_805FD9B8;     // 0x805FD9B8 (5.0f)
extern f32 lbl_805FD9BC;     // 0x805FD9BC (100.0f)
extern f32 lbl_805FD9C0;     // 0x805FD9C0 (20.0f)
}

static const u32 kHashExit = 0xA29C69BAu;  // "Exit"

typedef s32 (*IntVFn)(u8*);
typedef f32* (*PosVFn)(u8*, u8*, u8*);
typedef f32 (*FloatVFn)(u8*);
typedef void (*MtxVFn)(u8*, f32 (*)[4]);

// Normalizes (x, y, z) into out; the zero vector stays zero (the original guards frsqrte the same way).
static inline void NormalizeOrZero(f32* out, f32 x, f32 y, f32 z) {
    f32 len2 = z * z + (x * x + y * y);
    if (lbl_805FD468 == len2) {
        out[0] = lbl_805FD468;
        out[1] = lbl_805FD468;
        out[2] = lbl_805FD468;
    } else {
        f32 inv = 1.0f / sqrtf(len2);  // original: frsqrte with two Newton steps
        out[0] = x * inv;
        out[1] = y * inv;
        out[2] = z * inv;
    }
}

// Clamps a ray-scene result to the table index range 0..3.
static inline s32 ClampTableIndex(s32 v) {
    if (v < 0) return 0;
    if (v > 3) return 3;
    return v;
}

extern "C" {

// 0x8005DC04
// Per-frame update of a character's action state (+0xA04): runs the handler for the current state
// hash until one accepts it (otherwise shifts the queued states at +0xA24.. down and retries),
// then hands over to the base update fn_80053C2C.
void fn_8005DC04(u8* p, f32 dt) {
    // r4 (a second argument) is only forwarded unchanged to fn_80053C2C, which takes just (p, dt).
    if (*(u32*)(*(u8**)(p + 0x4) + 0x20) & 0x04000000) {
        fn_80053C2C(p, dt);
        return;
    }
    for (s32 i = 0; i < 4; i++) {
        if (*(f32*)(p + i * 0x20 + 0xB20) > lbl_805FD3F0) {
            fn_8005FD4C(p, p + 0xB04 + i * 0x20);
        }
    }
    for (;;) {
        s32 ok;
        switch (*(u32*)(p + 0xA04)) {
        case 0x7C498ED2u:
            ok = fn_8005E514(p, p + 0xA04);
            break;
        case 0x84BA5FEEu: {
            u8* t = *(u8**)(p + 0xA18);
            if (t != NULL && (*(IntVFn*)(*(u8**)t + 0x4C))(t) == 0) {
                *(u8**)(p + 0xA14) = *(u8**)(p + 0xA18);
            } else {
                *(void**)(p + 0xA14) = fn_80060A28(p);
            }
            u8* sel = *(u8**)(p + 0xA14);
            if (sel != NULL) {
                f32* pos = (*(PosVFn*)(*(u8**)sel + 0x44))(sel, p + 0x18, p);
                *(f32*)(p + 0xB88) = pos[0];
                *(f32*)(p + 0xB8C) = pos[1];
                *(f32*)(p + 0xB90) = pos[2];
                *(u32*)(p + 0xA08) = *(u32*)(p + 0xA08) | 8;
            }
            ok = 1;
            break;
        }
        case 0x7A6B66E7u:
            ok = fn_8005EED4(p, p + 0xA04);
            break;
        case 0x183384FFu:
            ok = fn_8005E694(p, p + 0xA04);
            break;
        case 0xCF423616u:
            ok = fn_8005E96C(p, p + 0xA04);
            break;
        case 0x9A056FFEu: {
            u8* t = *(u8**)(p + 0xA18);
            if (t != NULL && (*(IntVFn*)(*(u8**)t + 0x4C))(t) == 0) {
                *(u8**)(p + 0xA14) = *(u8**)(p + 0xA18);
            } else {
                *(void**)(p + 0xA14) = fn_80060A28(p);
            }
            u8* sel = *(u8**)(p + 0xA14);
            if (sel == NULL) {
                ok = 0;
            } else {
                f32* pos = (*(PosVFn*)(*(u8**)sel + 0x44))(sel, p + 0x18, p);
                *(f32*)(p + 0xB88) = pos[0];
                *(f32*)(p + 0xB8C) = pos[1];
                *(f32*)(p + 0xB90) = pos[2];
                *(u32*)(p + 0xA08) = *(u32*)(p + 0xA08) | 8;
                fn_8005E514(p, p + 0xA04);
                ok = 1;
            }
            break;
        }
        case 0x14DB00B3u:
            if (*(u8*)(p + 0xBDC) != 0) {
                ok = fn_8005E694(p, p + 0xA04);
            } else {
                ok = 0;
            }
            break;
        case 0x4553808Cu: {
            u8* t = *(u8**)(p + 0xA18);
            if (t == NULL || (*(IntVFn*)(*(u8**)t + 0x4C))(t) != 0) {
                ok = 0;
            } else {
                u8* sel = (u8*)fn_80060A28(p);
                *(u8**)(p + 0xA14) = sel;
                if (sel != NULL) {
                    u8* a = *(u8**)(sel + 0x4);
                    u8* b = *(u8**)(*(u8**)(p + 0xA18) + 0x4);
                    f32 r = *(f32*)(p + 0xA1C);
                    f32 d[3];
                    d[0] = *(f32*)(a + 0x0) - *(f32*)(b + 0x0);
                    d[1] = *(f32*)(a + 0x4) - *(f32*)(b + 0x4);
                    d[2] = *(f32*)(a + 0x8) - *(f32*)(b + 0x8);
                    if (d[2] * d[2] + (d[0] * d[0] + d[1] * d[1]) < r * r && fn_8005E694(p, p + 0xA04) != 0) {
                        ok = 1;
                    } else {
                        u8* c = *(u8**)(*(u8**)(p + 0xA18) + 0x4);
                        *(f32*)(p + 0xB88) = *(f32*)(c + 0x0);
                        *(f32*)(p + 0xB8C) = *(f32*)(c + 0x4);
                        *(f32*)(p + 0xB90) = *(f32*)(c + 0x8);
                        *(u32*)(p + 0xA08) = *(u32*)(p + 0xA08) | 8;
                        ok = 1;
                    }
                } else {
                    ok = 1;
                }
            }
            break;
        }
        default:
            ok = 0;
            break;
        }
        if (ok != 0) break;
        // No handler took the state: shift the 7 queued entries (0x20 bytes each) down by one.
        u8* e = p;
        for (s32 n = 7; n != 0; n--) {
            *(u32*)(e + 0xA04) = *(u32*)(e + 0xA24);
            *(u32*)(e + 0xA08) = *(u32*)(e + 0xA28);
            *(u8*)(e + 0xA0C) = *(u8*)(e + 0xA2C);
            *(u8*)(e + 0xA0D) = *(u8*)(e + 0xA2D);
            *(u16*)(e + 0xA0E) = *(u16*)(e + 0xA2E);
            *(u16*)(e + 0xA12) = *(u16*)(e + 0xA32);
            *(u8*)(e + 0xA11) = *(u8*)(e + 0xA31);
            *(u8*)(e + 0xA10) = *(u8*)(e + 0xA30);
            *(u8**)(e + 0xA14) = *(u8**)(e + 0xA34);
            *(u8**)(e + 0xA18) = *(u8**)(e + 0xA38);
            *(u32*)(e + 0xA1C) = *(u32*)(e + 0xA3C);
            *(f32*)(e + 0xA20) = *(f32*)(e + 0xA40);
            e += 0x20;
        }
        *(f32*)(p + 0xB00) = lbl_805FD404;
        if (*(u32*)(p + 0xA04) == 0x14DB00B3u) {
            *(u8*)(p + 0xBDC) = 1;
        }
    }
    u8** link = *(u8***)(p + 0xC24);
    if (link != NULL) {
        u8* o = *link;
        *(u32*)(o + 0x8) = *(u32*)(o + 0x8) & 0xFFFFFFFDu;
    }
    fn_8005E1B8(p);
    fn_80053C2C(p, dt);
}

// 0x8006697C
// Computes the half-extent vector at +0x188 (a perpendicular to the direction towards the player
// or towards +0x1A0 / +0x1AC, scaled by half of the value at +0xEC of the object at +0x228), then
// feeds the two points pos +/- vector to the colliders at +0x234 and +0x238.
void fn_8006697C(u8* p) {
    f32 n[3];
    f32 a[3];
    f32 hi[3];
    f32 lo[3];
    u32 flags = *(u32*)(p + 0x244);

    if (flags & 0x2000) {
        u8* pos = *(u8**)(p + 0x4);
        u8* tgt = *(u8**)(*(u8**)(lbl_803C4298 + 0x4) + 0x4);
        f32 px = *(f32*)(pos + 0x0);
        f32 py = *(f32*)(pos + 0x4);
        f32 pz = *(f32*)(pos + 0x8);
        a[0] = *(f32*)(tgt + 0x0) - px;
        a[1] = *(f32*)(tgt + 0x4) - py;
        a[2] = *(f32*)(tgt + 0x8) - pz;
        f32 cx, cy, cz;
        if (flags & 0x20000) {
            f32 vx = *(f32*)(p + 0x1A0) - px;
            f32 vy = *(f32*)(p + 0x1A4) - py;
            f32 vz = *(f32*)(p + 0x1A8) - pz;
            cx = a[1] * vz - a[2] * vy;
            cy = a[2] * vx - a[0] * vz;
            cz = a[0] * vy - a[1] * vx;
        } else {
            f32 wx = *(f32*)(p + 0x1AC);
            f32 wy = *(f32*)(p + 0x1B0);
            f32 wz = *(f32*)(p + 0x1B4);
            cx = a[1] * wz - a[2] * wy;
            cy = a[2] * wx - a[0] * wz;
            cz = a[0] * wy - a[1] * wx;
        }
        NormalizeOrZero(n, cx, cy, cz);
        f32 s = lbl_805FD480 * *(f32*)(*(u8**)(p + 0x228) + 0xEC);
        *(f32*)(p + 0x188) = n[0] * s;
        *(f32*)(p + 0x18C) = n[1] * s;
        *(f32*)(p + 0x190) = n[2] * s;
    } else {
        f32 zero = lbl_805FD468;
        f32 one = lbl_805FD484;
        f32 c0, c1, c2;
        if (flags & 0x20000) {
            u8* pos = *(u8**)(p + 0x4);
            f32 vx = *(f32*)(p + 0x1A0) - *(f32*)(pos + 0x0);
            f32 vy = *(f32*)(p + 0x1A4) - *(f32*)(pos + 0x4);
            f32 vz = *(f32*)(p + 0x1A8) - *(f32*)(pos + 0x8);
            // cross product of v with the axis (0, 0, 1)
            c0 = vy * one - vz * zero;
            c1 = vz * zero - vx * one;
            c2 = vx * zero - vy * zero;
        } else {
            f32 wx = *(f32*)(p + 0x1AC);
            f32 wy = *(f32*)(p + 0x1B0);
            f32 wz = *(f32*)(p + 0x1B4);
            c0 = wy * one - wz * zero;
            c1 = wz * zero - wx * one;
            c2 = wx * zero - wy * zero;
        }
        *(f32*)(p + 0x188) = c0;
        *(f32*)(p + 0x18C) = c1;
        *(f32*)(p + 0x190) = c2;
        NormalizeOrZero(n, *(f32*)(p + 0x188), *(f32*)(p + 0x18C), *(f32*)(p + 0x190));
        *(f32*)(p + 0x188) = n[0];
        *(f32*)(p + 0x18C) = n[1];
        *(f32*)(p + 0x190) = n[2];
        f32 s = lbl_805FD480 * *(f32*)(*(u8**)(p + 0x228) + 0xEC);
        *(f32*)(p + 0x188) = n[0] * s;
        *(f32*)(p + 0x18C) = n[1] * s;
        *(f32*)(p + 0x190) = n[2] * s;
    }
    u8* pos = *(u8**)(p + 0x4);
    hi[0] = *(f32*)(pos + 0x0) + *(f32*)(p + 0x188);
    hi[1] = *(f32*)(pos + 0x4) + *(f32*)(p + 0x18C);
    hi[2] = *(f32*)(pos + 0x8) + *(f32*)(p + 0x190);
    pos = *(u8**)(p + 0x4);
    lo[0] = *(f32*)(pos + 0x0) - *(f32*)(p + 0x188);
    lo[1] = *(f32*)(pos + 0x4) - *(f32*)(p + 0x18C);
    lo[2] = *(f32*)(pos + 0x8) - *(f32*)(p + 0x190);
    u8* col = *(u8**)(p + 0x234);
    if (col != NULL) {
        fn_801C4B90(col, hi, lo, 0, 0, *(u8**)(*(u8**)col + 0x4));
    }
    col = *(u8**)(p + 0x238);
    if (col != NULL) {
        fn_801C5CC0(col, hi, lo);
    }
}

// 0x8006DCAC
// Handles a touch/hit of object q (target at +0x5C) on p: fires the trigger actions, keeps the
// five (target, time) slots at +0x1D4 and, once enough distinct targets have been collected,
// runs the "all hit" actions and notifies each target through fn_8006DA84.
void fn_8006DCAC(u8* p, u8* q) {
    u8* t = *(u8**)(q + 0x5C);
    s32 i;

    if (t != NULL && *(s32*)(*(u8**)(p + 0x9C) + 0x8) == (s32)0x6DB80004 &&
        (*(IntVFn*)(*(u8**)t + 0x4C))(t) != 0) {
        return;
    }
    if (t != NULL && *(s32*)(*(u8**)(p + 0x9C) + 0x8) == (s32)0x6DB80004) {
        u8** a = *(u8***)(p + 0xA4);
        if (a != NULL) {
            u8* b = *a;
            if (b != NULL) {
                s32 idx = *(s32*)(b + 0x4);
                u8* ent = NULL;
                if (idx > -1) {
                    ent = **(u8***)b + idx * 0x30;
                }
                s32 key = 0;
                if (ent != NULL) key = *(s32*)(ent + 0x4);
                if ((u32)key != 0xA75C2DA2u) return;
            }
        }
    }
    if ((*(u32*)(p + 0x1D0) >> 27) & 0xF) {
        u8* tt = *(u8**)(q + 0x5C);
        if (tt != NULL) {
            *(u8**)(p + 0x1AC) = tt;
            TriggerDoAction(p + 0x1D0, p, tt);
            f32 f = *(f32*)(p + 0x1BC);
            *(u32*)(p + 0x1AC) = 0;
            if (f > lbl_805FD598) {
                *(f32*)(p + 0x1C0) = f;
                for (i = 0; i < 5; i++) {
                    u8* e = *(u8**)(p + i * 8 + 0x1D4);
                    if (e != NULL) {
                        *(u32*)(p + i * 8 + 0x1D4) = 0;
                        u8* sub = *(u8**)(p + 0x9C);
                        if (sub != NULL && ((*(u32*)(sub + 0xA4) >> 27) & 0xF)) {
                            TriggerDoAction(sub + 0xA4, e, p);
                        }
                        if ((*(u32*)(p + 0x74) >> 27) & 0xF) {
                            TriggerDoAction(p + 0x74, e, p);
                        }
                        EventSend(gEventMgr, p, p, kHashExit, 0, 0, 1, 0);
                    }
                }
            }
        }
    }
    if (t == NULL) return;

    if (*(s32*)(p + 0x208) != 0) {
        if (*(u32*)(p + 0x1B0) & 0x800) {
            for (i = 0; i < 5; i++) {
                if (*(u8**)(p + i * 8 + 0x1D4) == t) {
                    u8* sub = *(u8**)(p + 0x9C);
                    if (sub != NULL && ((*(u32*)(sub + 0xA0) >> 27) & 0xF)) {
                        TriggerDoAction(sub + 0xA0, t, p);
                    }
                    if ((*(u32*)(p + 0x70) >> 27) & 0xF) {
                        TriggerDoAction(p + 0x70, t, p);
                    }
                    *(u32*)(p + i * 8 + 0x1D8) = *(u32*)(p + 0x20C);
                    return;
                }
            }
        }
        // Find the target's slot (and remember the last empty one).
        s32 emptySlot = -1;
        s32 found = 0;
        for (i = 0; i < 5; i++) {
            u8* e = *(u8**)(p + i * 8 + 0x1D4);
            if (e == NULL) {
                emptySlot = i;
            } else if (e == t) {
                *(u32*)(p + i * 8 + 0x1D8) = *(u32*)(p + 0x20C);
                found = 1;
                break;
            }
        }
        if (found == 0) {
            if (emptySlot == -1) emptySlot = 0;
            *(u8**)(p + emptySlot * 8 + 0x1D4) = t;
            *(u32*)(p + emptySlot * 8 + 0x1D8) = *(u32*)(p + 0x20C);
        }
        if (*(u32*)(p + 0x1B0) & 0x800) return;
        if ((u32)fn_8006D9A0(p) < *(u32*)(p + 0x208)) return;
        f32 f = *(f32*)(p + 0x1BC);
        *(u32*)(p + 0x1B0) = *(u32*)(p + 0x1B0) | 0x800;
        if (f > lbl_805FD598) {
            *(f32*)(p + 0x1C0) = f;
            for (i = 0; i < 5; i++) {
                u8* e = *(u8**)(p + i * 8 + 0x1D4);
                if (e != NULL) {
                    *(u32*)(p + i * 8 + 0x1D4) = 0;
                    u8* sub = *(u8**)(p + 0x9C);
                    if (sub != NULL && ((*(u32*)(sub + 0xA4) >> 27) & 0xF)) {
                        TriggerDoAction(sub + 0xA4, e, p);
                    }
                    if ((*(u32*)(p + 0x74) >> 27) & 0xF) {
                        TriggerDoAction(p + 0x74, e, p);
                    }
                    EventSend(gEventMgr, p, p, kHashExit, 0, 0, 1, 0);
                }
            }
        }
        TriggerDoAction(p + 0x1C4, p, p);
        for (i = 0; i < 5; i++) {
            u8* e = *(u8**)(p + i * 8 + 0x1D4);
            if (e != NULL) {
                fn_8006DA84(p, e);
            }
        }
    } else {
        s32 used = 0;
        for (i = 0; i < 5; i++) {
            u8* e = *(u8**)(p + i * 8 + 0x1D4);
            if (e == t) {
                u8* sub = *(u8**)(p + 0x9C);
                if (sub != NULL && ((*(u32*)(sub + 0xA0) >> 27) & 0xF)) {
                    TriggerDoAction(sub + 0xA0, t, p);
                }
                if ((*(u32*)(p + 0x70) >> 27) & 0xF) {
                    TriggerDoAction(p + 0x70, t, p);
                }
                *(u32*)(p + i * 8 + 0x1D8) = *(u32*)(p + 0x20C);
                return;
            }
            if (e != NULL) used++;
        }
        if ((*(u32*)(p + 0x1B0) & 0x20) && used > 0) return;
        for (i = 0; i < 5; i++) {
            if (*(u8**)(p + i * 8 + 0x1D4) == NULL &&
                (u32)(*(s32*)(p + 0x20C) - *(s32*)(p + i * 8 + 0x1D8)) > 2u) {
                *(u8**)(p + i * 8 + 0x1D4) = t;
                *(u32*)(p + i * 8 + 0x1D8) = *(u32*)(p + 0x20C);
                fn_8006DA84(p, t);
                return;
            }
        }
    }
}

// 0x80088004
// Per-frame update of the target-selection mode: projects the stick offset to screen coordinates
// for the camera (+0x354), picks the on-screen target (list at +0x94) or a fallback target when
// the confirm button is held, sends the select / deselect events, then updates the target scale.
// The second argument (a float) is not used.
void fn_80088004(u8* p, f32 dt) {
    f32 w = (f32)gScreenWidth;
    f32 h = (f32)gScreenHeight;
    f32 half = lbl_805FD940;
    f32 sy = *(f32*)(p + 0x474) * *(f32*)(p + 0x454);
    f32 sx = *(f32*)(p + 0x470) * *(f32*)(p + 0x450);
    f32 halfH = half * h;
    f32 halfW = half * w;
    f32 screenY = halfH + halfH * sy;
    f32 screenX = halfW + halfW * sx;
    u8* cam;
    s32 pressed;
    s32 hold;

    // The original also passes ten more float registers (leftovers); the callee takes only x, y.
    fn_8008C148(*(u8**)(p + 0x354), screenX, screenY);

    pressed = 0;
    hold = 0;
    if (*(s32*)(*(u8**)(p + 0x354) + 0x264) != 1) {
        s32 pad = *(s32*)(lbl_803C4298 + 0x38);
        if (PadButtonHeld(pad * 16 + 0x1089) != 0 || PadButtonHeld(pad * 16 + 0x108A) != 0) {
            pressed = 1;
        } else if (PadButtonHeld(pad * 16 + 0x103A) != 0) {
            pressed = 1;
        }
    }
    if (*(u8*)(p + 0x7C0) != 0) {
        f32 v = *(f32*)(p + 0x7C4);
        if (v > lbl_805FD928) {
            *(f32*)(p + 0x738) = v;
        }
    }
    if (*(u8*)(p + 0x7C0) != 0) {
        hold = 1;
    }
    if (pressed != 0) {
        u8* found;
        u8* fallback;
        s32 hit;
        EventSend(gEventMgr, *(u8**)(p + 0x354), NULL, 0xEA13A4E7u, 0, 0, 1, 0);
        found = NULL;
        fallback = NULL;
        hit = 0;
        {
            f32 sxy[2];
            sxy[0] = lbl_805FD928;
            sxy[1] = lbl_805FD928;
            for (s32 i = 0; i < 0x19; i++) {
                u8* e = *(u8**)(p + 0x94 + i * 4);
                if (e != NULL && *(s32*)(e + 0x264) != 1) {
                    u8* ep = *(u8**)(e + 0x4);
                    f32 pos[3];
                    pos[0] = *(f32*)(ep + 0x0);
                    pos[1] = *(f32*)(ep + 0x4);
                    pos[2] = *(f32*)(ep + 0x8);
                    fn_80073A7C(NULL, pos, &sxy[0], &sxy[1]);
                    if (fn_8008C1BC(*(u8**)(p + 0x354), (s32)sxy[0], (s32)sxy[1]) != 0) {
                        hit = 1;
                        found = *(u8**)(p + 0x94 + i * 4);
                        break;
                    }
                }
            }
        }
        if (hit != 0) {
            if (fn_8008D67C(found) != 0) {
                fn_8008C414(*(u8**)(p + 0x354), found);
                fn_8008C2EC(*(u8**)(p + 0x354));
                *(f32*)(p + 0x484) = *(f32*)(p + 0x484) + lbl_805FD95C;
                // r3 here is the (ignored) result of fn_8008C2EC.
                s32 idx = ClampTableIndex(RaySceneTest(NULL));
                f32 zero = lbl_805FD928;
                f32 lim = lbl_803469A8[idx];
                f32 cur = *(f32*)(p + 0x484);
                f32 res;
                if (zero < lim) {
                    if (cur < zero) {
                        res = zero;
                    } else {
                        if (cur > lim) {
                            // keep lim
                        } else {
                            lim = cur;
                        }
                        res = lim;
                    }
                } else {
                    if (cur < lim) {
                        res = lim;
                    } else {
                        if (cur > zero) {
                            // keep zero
                        } else {
                            zero = cur;
                        }
                        res = zero;
                    }
                }
                *(f32*)(p + 0x484) = res;
                fn_800712B4();
                fn_8008D670(found);
                EventSend(gEventMgr, found, NULL, 0x94662077u, 0, 0, 1, 0);
                EventSend(gEventMgr, *(u8**)(p + 0x354), NULL, 0xA221B5ACu, 0, 0, 1, 0);
            }
        } else if (fn_8008928C(p, &fallback) != 0) {
            fn_8008C414(*(u8**)(p + 0x354), fallback);
            s32 cur = *(s32*)(*(u8**)(p + 0x354) + 0x370);
            if (cur != 0) {
                s32 other = *(s32*)(fallback + 0x268);
                if (other != 0 && cur != other) {
                    u8* popped = (u8*)(intptr_t)fn_8008C0B4(*(u8**)(p + 0x354)); // lint:ok (original returns a pointer; old def returns s32)
                    if (popped != NULL) {
                        EventSend(gEventMgr, popped, NULL, 0xEEC129BFu, 0, 0, 1, 0);
                    }
                }
            }
            fn_8008D670(fallback);
            EventSend(gEventMgr, fallback, NULL, 0x94662077u, 0, 0, 1, 0);
            EventSend(gEventMgr, *(u8**)(p + 0x354), NULL, 0xA221B5ACu, 0, 0, 1, 0);
        }
    } else {
        cam = *(u8**)(p + 0x354);
        if (*(s32*)(cam + 0x264) != 1 && *(s32*)(cam + 0x35C) != 0) {
            fn_8008C380(cam);
        }
    }
    if (pressed == 0) {
        EventSend(gEventMgr, *(u8**)(p + 0x354), NULL, 0x13161A1Eu, 0, 0, 1, 0);
    }
    if (hold != 0) {
        fn_80089B30(p);
    }
    fn_80070FAC(p, (*(FloatVFn*)(*(u8**)(p + 0x4) + 0x64))(p) / lbl_805FD944);
}

// 0x8008A768
// Weighted random pick: rolls a number below the sum of seven table weights (each indexed by a
// ray-scene result clamped to 0..3), then walks the cumulative weights to choose which of the
// outcomes (*a, *b) = (0,0..4) or (*a, *c) = (1,1..2) to report.
void fn_8008A768(u8* p, s32* a, s32* b, s32* c) {
    // RaySceneTest ignores its argument; later calls get the previous result in r3 (leftover).
    s32 i0 = ClampTableIndex(RaySceneTest(p));
    s32 i1 = ClampTableIndex(RaySceneTest(NULL));
    s32 i2 = ClampTableIndex(RaySceneTest(NULL));
    s32 i3 = ClampTableIndex(RaySceneTest(NULL));
    s32 i4 = ClampTableIndex(RaySceneTest(NULL));
    s32 i5 = ClampTableIndex(RaySceneTest(NULL));
    s32 i6 = ClampTableIndex(RaySceneTest(NULL));
    u32 roll = RandomU32();
    u32 total = lbl_80346928[i2] + (lbl_80346948[i0] + lbl_80346938[i1]) +
                (lbl_80346918[i3] + lbl_80346908[i4] + (lbl_803468E8[i5] + lbl_803468F8[i6]));
    s32 r = (s32)(roll % total);

    if (r < lbl_803468E8[ClampTableIndex(RaySceneTest(NULL))]) {
        *a = 0;
        *b = 0;
        return;
    }
    r -= lbl_803468E8[ClampTableIndex(RaySceneTest(NULL))];
    if (r < lbl_803468F8[ClampTableIndex(RaySceneTest(NULL))]) {
        *a = 0;
        *b = 1;
        return;
    }
    r -= lbl_803468F8[ClampTableIndex(RaySceneTest(NULL))];
    if (r < lbl_80346908[ClampTableIndex(RaySceneTest(NULL))]) {
        *a = 0;
        *b = 2;
        return;
    }
    r -= lbl_80346908[ClampTableIndex(RaySceneTest(NULL))];
    if (r < lbl_80346918[ClampTableIndex(RaySceneTest(NULL))]) {
        *a = 0;
        *b = 3;
        return;
    }
    r -= lbl_80346918[ClampTableIndex(RaySceneTest(NULL))];
    if (r < lbl_80346928[ClampTableIndex(RaySceneTest(NULL))]) {
        *a = 0;
        *b = 4;
        return;
    }
    r -= lbl_80346928[ClampTableIndex(RaySceneTest(NULL))];
    if (r < lbl_80346938[ClampTableIndex(RaySceneTest(NULL))]) {
        *a = 1;
        *c = 1;
        return;
    }
    *a = 1;
    *c = 2;
}

// 0x8008C41C
// Aims the camera pick at screen point (x, y) (clamped inside a 5% / 20% margin): re-reads the
// camera matrix, bilinearly blends the two sets of four corner rays of the current camera at that
// point, and places the object's position where the blended near/far segment crosses y = 0.
void fn_8008C41C(u8* p, f32 x, f32 y) {
    f32 w = (f32)gScreenWidth;
    f32 h = (f32)gScreenHeight;
    f32 xmin = (lbl_805FD9B8 * (f32)gScreenWidth) / lbl_805FD9BC;
    f32 ymin = (lbl_805FD9B8 * (f32)gScreenHeight) / lbl_805FD9BC;
    f32 xmax = w - (lbl_805FD9B8 * (f32)gScreenWidth) / lbl_805FD9BC;
    f32 ymax = h - (lbl_805FD9C0 * (f32)gScreenHeight) / lbl_805FD9BC;
    f32 mtx[3][4];
    u8* gcam;

    if (x < xmin) {
        x = xmin;
    } else if (x > xmax) {
        x = xmax;
    }
    if (y < ymin) {
        y = ymin;
    } else if (y > ymax) {
        y = ymax;
    }
    {
        u8* obj = *(u8**)(lbl_803C4298 + 0x4);
        (*(MtxVFn*)(*(u8**)obj + 0x38))(obj, mtx);
    }
    CameraSetMatrix(lbl_8047C640, mtx);
    fn_80009840(lbl_803C4298);

    gcam = (u8*)lbl_805FB5F8;
    if (gcam != NULL) {
        f32 u = x / (f32)gScreenWidth;
        f32 v = lbl_805FD9B4 - y / (f32)gScreenHeight;
        f32 nu = lbl_805FD9B4 - u;
        f32 w00 = u * v;
        f32 w10 = nu * v;
        f32 nv = lbl_805FD9B4 - v;
        f32 w01 = u * nv;
        f32 w11 = nu * nv;

        // Near corner: blend of the four corner points at +0x120.. .
        f32 d0 = *(f32*)(gcam + 0x12C) * w00;
        f32 a0 = *(f32*)(gcam + 0x120) * w10;
        f32 d1 = *(f32*)(gcam + 0x130) * w00;
        f32 a1 = *(f32*)(gcam + 0x124) * w10;
        f32 g0 = *(f32*)(gcam + 0x138) * w01;
        f32 a2 = *(f32*)(gcam + 0x128) * w10;
        f32 d2 = *(f32*)(gcam + 0x134) * w00;
        f32 j0 = *(f32*)(gcam + 0x144) * w11;
        f32 j1 = *(f32*)(gcam + 0x148) * w11;
        f32 j2 = *(f32*)(gcam + 0x14C) * w11;
        f32 g1 = *(f32*)(gcam + 0x13C) * w01;
        f32 g2 = *(f32*)(gcam + 0x140) * w01;
        f32 nx = (d0 + a0) + g0 + j0;
        f32 ny = (d1 + a1) + g1 + j1;
        f32 nz = (d2 + a2) + g2 + j2;

        // Far corner: blend of the four corner points at +0x150.. .
        f32 p2 = *(f32*)(gcam + 0x15C) * w00;
        f32 m0 = *(f32*)(gcam + 0x150) * w10;
        f32 m1 = *(f32*)(gcam + 0x154) * w10;
        f32 q1 = *(f32*)(gcam + 0x160) * w00;
        f32 m2 = *(f32*)(gcam + 0x158) * w10;
        f32 s0 = *(f32*)(gcam + 0x168) * w01;
        f32 r1 = *(f32*)(gcam + 0x164) * w00;
        f32 s1 = *(f32*)(gcam + 0x16C) * w01;
        f32 s2 = *(f32*)(gcam + 0x170) * w01;
        f32 v1 = *(f32*)(gcam + 0x174) * w11;
        f32 v2 = *(f32*)(gcam + 0x178) * w11;
        f32 v3 = *(f32*)(gcam + 0x17C) * w11;
        f32 fx = ((p2 + m0) + s0) + v1;
        f32 fy = ((q1 + m1) + s1) + v2;
        f32 fz = ((r1 + m2) + s2) + v3;

        f32 ay = fabsf(ny);
        f32 by = fabsf(fy);
        f32 dx = fx - nx;
        f32 dy = fy - ny;
        f32 dz = fz - nz;
        f32 t = ay / (ay + by);
        u8* pos = *(u8**)(p + 0x4);
        *(f32*)(pos + 0x0) = nx + dx * t;
        *(f32*)(pos + 0x4) = ny + dy * t;
        *(f32*)(pos + 0x8) = nz + dz * t;
        pos = *(u8**)(p + 0x4);
        *(u32*)(pos + 0x20) = *(u32*)(pos + 0x20) | 0x8000;
        pos = *(u8**)(p + 0x4);
        *(u32*)(pos + 0x20) = *(u32*)(pos + 0x20) | 0x10000000;
        pos = *(u8**)(p + 0x4);
        *(u32*)(pos + 0x20) = *(u32*)(pos + 0x20) | 0x4000;
        pos = *(u8**)(p + 0x4);
        *(u32*)(pos + 0x20) = *(u32*)(pos + 0x20) | 0x10000000;
    }
}

}  // extern "C"
