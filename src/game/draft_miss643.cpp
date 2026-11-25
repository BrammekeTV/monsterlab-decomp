// Reactor / zone-trigger object updates, screen-ray unprojection and the leak-plate controller.
#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" void* gEventMgr;  // 0x805FB6C8

extern "C" {

void CameraSetMatrix(void* camera, const f32 mtx[3][4]);                                      // 0x801D6814
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
void* PhysicsBodyGet(void* handle);                                                          // 0x801B1840
u32 RandomU32(void);                                                                         // 0x80143460
s32 RaySceneTest(void* ray);                                                                 // 0x80070458
s32 TriggerDoAction(void* action, void* a1, void* a2);                                       // 0x801E0054
void ZoneEffectsUpdate(void* effects, f32 dt);                                               // 0x801E97FC
BOOL ZoneIsLoaded(void* world, s32 zone);                                                    // 0x8012F408
void fn_80009840(u8* p);                                                                     // 0x80009840
void fn_8006EF78(u8* p, u8* out);                                                                     // 0x8006EF78
f32 fn_800734B4(u8* camera, f32* vec);                                                       // 0x800734B4
s32 fn_80097064(f32* dir);                                                                   // 0x80097064
void fn_80097BCC(u8* out, u8* obj);                                                          // 0x80097BCC
s32 fn_800972CC(u8* p);                                                                      // 0x800972CC
void fn_800980F8(u8* p, s32 b);                                                              // 0x800980F8
void fn_8009875C(u8* obj, f32* color);                                                       // 0x8009875C
u8* fn_801ABF0C(u8* table, u32 hash, u8* key);                                               // 0x801ABF0C
u8* fn_801AC024(u8* table, u32 hash, u8* key, u8* prev);                                     // 0x801AC024
void fn_801ACBEC(void* p, f32 dt);                                                           // 0x801ACBEC
f64 floor(f64 x);                                                                      // 0x8033ED6C (floor)
f64 fn_8033F1F4(f64 x);                                                                      // 0x8033F1F4 (acos)

extern s32 gScreenHeight;      // 0x805F938C
extern s32 gScreenWidth;       // 0x805F9388
extern u8 gWorldState[];       // 0x803E6D00
extern u8 lbl_803C4298[];      // 0x803C4298 (+4: object with the camera-matrix getter)
extern u8 lbl_8047C640[];      // 0x8047C640 (camera)
extern void* lbl_805FB5F8;     // 0x805FB5F8 (current camera)

extern f32 lbl_805FD300;  // 0.0f
extern f32 lbl_805FD304;  // 1.0f
extern f32 lbl_805FD348;  // 0.8f
extern f32 lbl_805FD598;  // 0.0f
extern f32 lbl_805FD5F0;  // 1.0f
extern f32 lbl_805FDB28;  // 0.0f
extern f32 lbl_805FDB2C;  // 2.3283064e-10f (1 / 2^32)
extern f32 lbl_805FDB38;  // -1.0f
extern f32 lbl_805FDB3C;  // 50.0f
extern f32 lbl_805FDB64;  // 1.0f
extern f32 lbl_805FDB74;  // 0.125f
extern f32 lbl_805FDB84;  // 2 / pi
extern f32 lbl_805FDB88;  // 4.0f
extern f32 lbl_805FDB8C;  // 11.0f

extern s32 lbl_80347090[];  // per-difficulty mode table
extern f32 lbl_803470B0[];
extern f32 lbl_803470C0[];
extern f32 lbl_803470D0[];
extern f32 lbl_803470E0[];
extern s32 lbl_80347150[];
extern s32 lbl_80347160[];
extern s32 lbl_80347170[];
extern s32 lbl_80347180[];

static const u32 kHashTableA = 0xFC17C3F3u;
static const u32 kHashTableB = 0x5AE1E03Eu;
static const u32 kHashExit = 0xA29C69BAu;      // "Exit"
static const u32 kHashActivate = 0x705598E1u;  // "Activate"
static const u32 kSndLeak1 = 0x10461444u;      // "reactor_leak_level_1_leak_01"
static const u32 kSndLeak2 = 0x29CB2881u;      // "reactor_leak_level_2_leak_01"
static const u32 kSndLeak3 = 0x3EB03CC2u;      // "reactor_leak_level_3_leak_01"
static const u32 kSndRattle = 0x1228CC56u;     // "reactor_leak_plate_rattle_01"

// 0x80059A88
// Decides whether the target at +0x6BC is out of range of this object (distance versus the two
// radii plus 0.8). Out of range sets flag 4 and a velocity along the +0xB0 scale; otherwise it
// clears the flag, zeroes the velocity and stores the product of the table-B multipliers at +0xE8.
void fn_80059A88(u8* p) {
    if (*(u32*)(p + 0x838) & 0x8000) return;
    u8** ref = *(u8***)(p + 0xA4);
    if (ref == NULL) return;
    u8* obj = *ref;
    if (obj == NULL) return;

    s32 idx = *(s32*)(obj + 0x4);
    u8* entry = NULL;
    if (idx > -1) entry = **(u8***)obj + idx * 0x30;
    s32 v = 0;
    if (entry != NULL) v = *(s32*)(entry + 0x4);
    if (v == 0) return;

    u8* table = *(u8**)(p + 0x84);
    u8* hit = NULL;
    f32 scale = lbl_805FD304;
    if (table != NULL) scale = *(f32*)(table + 0x8);
    if (table != NULL) hit = fn_801ABF0C(table, kHashTableA, *(u8**)(p + 0xA0));

    bool far = false;
    if (hit != NULL) {
        u8* target = *(u8**)(p + 0x6BC);
        if (target == NULL) {
            far = true;
        } else if (!ZoneIsLoaded(gWorldState, *(s32*)(p + 0x6C0))) {
            far = true;
        } else {
            f32 tp[3];
            f32 tr;
            if (*(u32*)(*(u8**)(target + 0x4) + 0x1C) & 0x10) {
                u8* body = *(u8**)(target + 0x8C);
                if (body != NULL && PhysicsBodyGet(body) != NULL) {
                    u8* b = (u8*)PhysicsBodyGet(*(u8**)(target + 0x8C));
                    tp[0] = *(f32*)(b + 0x14);
                    tp[1] = *(f32*)(b + 0x18);
                    tp[2] = *(f32*)(b + 0x1C);
                } else {
                    tp[0] = *(f32*)(target + 0x18);
                    tp[1] = *(f32*)(target + 0x1C);
                    tp[2] = *(f32*)(target + 0x20);
                }
                target = *(u8**)(p + 0x6BC);
                body = *(u8**)(target + 0x8C);
                if (body != NULL && PhysicsBodyGet(body) != NULL) {
                    tr = *(f32*)((u8*)PhysicsBodyGet(*(u8**)(target + 0x8C)) + 0x3C);
                } else {
                    tr = *(f32*)(target + 0x14);
                }
            } else {
                tp[0] = *(f32*)(target + 0x18);
                tr = *(f32*)(target + 0x14);
                tp[1] = *(f32*)(target + 0x1C);
                tp[2] = *(f32*)(target + 0x20);
            }

            f32 sp[3];
            u8* body = *(u8**)(p + 0x8C);
            if (body != NULL && PhysicsBodyGet(body) != NULL) {
                u8* b = (u8*)PhysicsBodyGet(*(u8**)(p + 0x8C));
                sp[0] = *(f32*)(b + 0x14);
                sp[1] = *(f32*)(b + 0x18);
                sp[2] = *(f32*)(b + 0x1C);
            } else {
                sp[0] = *(f32*)(p + 0x18);
                sp[1] = *(f32*)(p + 0x1C);
                sp[2] = *(f32*)(p + 0x20);
            }

            f32 dx = tp[0] - sp[0];
            f32 dy = tp[1] - sp[1];
            f32 dz = tp[2] - sp[2];
            f32 dist = lbl_805FD300;
            f32 d2 = (dz * dz) + ((dx * dx) + (dy * dy));
            if (!(d2 <= 0.0f)) dist = sqrtf(d2);

            f32 sr;
            body = *(u8**)(p + 0x8C);
            if (body != NULL && PhysicsBodyGet(body) != NULL) {
                sr = *(f32*)((u8*)PhysicsBodyGet(*(u8**)(p + 0x8C)) + 0x3C);
            } else {
                sr = *(f32*)(p + 0x14);
            }
            far = dist > lbl_805FD348 + (tr + sr);
        }
    }

    if (far) {
        f32 zero = lbl_805FD300;
        f32 one = lbl_805FD304;
        *(u32*)(p + 0x838) |= 4;
        f32 k = *(f32*)(hit + 0x10) * *(f32*)(*(u8**)(p + 0xA0) + 0xB0);
        *(f32*)(p + 0x63C) = zero * k;
        *(f32*)(p + 0x640) = one * k;
        *(f32*)(p + 0x644) = zero * k;
        return;
    }

    f32 zero = lbl_805FD300;
    *(u32*)(p + 0x838) &= 0xFFFFFFFBu;
    *(f32*)(p + 0x63C) = zero;
    *(f32*)(p + 0x640) = zero;
    *(f32*)(p + 0x644) = zero;
    if (table != NULL) {
        u8* e = fn_801ABF0C(table, kHashTableB, *(u8**)(p + 0xA0));
        while (e != NULL) {
            scale *= *(f32*)(e + 0x10);
            e = fn_801AC024(table, kHashTableB, *(u8**)(p + 0xA0), e);
        }
    }
    *(f32*)(*(u8**)(p + 0xA0) + 0xE8) = scale;
}

// Fires the "release" actions for one pending contact slot: the contact's action word at +0xA4,
// this object's own action word at +0x74, then the Exit event.
static void ReleaseContact(u8* p, u8* target) {
    u8* contact = *(u8**)(p + 0x9C);
    if (contact != NULL && ((*(u32*)(contact + 0xA4) >> 27) & 0xF)) {
        TriggerDoAction(contact + 0xA4, target, p);
    }
    if ((*(u32*)(p + 0x74) >> 27) & 0xF) {
        TriggerDoAction(p + 0x74, target, p);
    }
    EventSend(gEventMgr, p, p, kHashExit, 0, 0, 1, 0);
}

// 0x8006E1A0
// Per-frame update of a trigger zone: ticks the cooldown timers, expires stale contact slots
// (5 slots of {object, frame stamp} from +0x1D4), fires release actions and the activation
// actions, then updates the zone effects.
void fn_8006E1A0(u8* p, f32 dt) {
    if (!(*(u32*)(*(u8**)(p + 0x4) + 0x20) & 1)) return;

    *(u32*)(p + 0x20C) += 1;
    for (s32 i = 0; i < 2; i++) {
        if (*(s32*)(p + 0x17C + i * 4) != 0) fn_8006EF78(p, *(u8**)(p + 0x17C + i * 4));  // r4 = the slot just tested
    }
    if (*(u32*)(*(u8**)(p + 0x4) + 0x20) & 0x04000000) return;

    f32 t = *(f32*)(p + 0x1B8);
    if (t > lbl_805FD598) *(f32*)(p + 0x1B8) = t - dt;
    t = *(f32*)(p + 0x1C0);
    if (t > lbl_805FD598) {
        *(f32*)(p + 0x1C0) = t - dt;
        return;
    }

    if (*(s32*)(p + 0x208) != 0 && (*(u32*)(p + 0x1B0) & 0x800)) {
        s32 live = 0;
        for (s32 i = 0; i < 5; i++) {
            if (*(u8**)(p + 0x1D4 + i * 8) != NULL) {
                if ((u32)(*(u32*)(p + 0x20C) - *(u32*)(p + 0x1D8 + i * 8)) > 1u) {
                    *(u8**)(p + 0x1D4 + i * 8) = NULL;
                } else {
                    live++;
                }
            }
        }
        if (live == 0) {
            *(u32*)(p + 0x1B0) &= ~0x800u;
            TriggerDoAction(p + 0x1C8, p, p);
            return;
        }
    } else {
        for (s32 i = 0; i < 5; i++) {
            u8* obj = *(u8**)(p + 0x1D4 + i * 8);
            if (obj != NULL && (u32)(*(u32*)(p + 0x20C) - *(u32*)(p + 0x1D8 + i * 8)) > 1u) {
                ReleaseContact(p, obj);
                *(u8**)(p + 0x1D4 + i * 8) = NULL;
                return;
            }
        }
    }

    u32 flags = *(u32*)(p + 0x1B0);
    bool ready = false;
    if (flags & 0x200) {
        if (*(f32*)(p + 0x1C0) <= lbl_805FD598) ready = true;
    }
    if (!ready) {
        if (flags & 0x400) {
            void* fx = *(void**)(p + 0xA4);
            if (fx != NULL) fn_801ACBEC(fx, dt);
        }
        return;
    }

    if (flags & 0x2) {
        s32* fx = *(s32**)(p + 0xA4);
        if (fx != NULL && *fx != 0) {
            EventSend(gEventMgr, p, p, kHashActivate, 0, 0, 1, 0);
            *(u32*)(p + 0x1B0) &= ~2u;
        }
    }
    if (*(u32*)(p + 0x1B0) & 0x400) {
        void* fx = *(void**)(p + 0xA4);
        if (fx != NULL) fn_801ACBEC(fx, dt);
    }
    if (TriggerDoAction(p + 0x1CC, p, NULL) != 0) {
        TriggerDoAction(p + 0x1D0, p, NULL);
        f32 cooldown = *(f32*)(p + 0x1BC);
        if (cooldown > lbl_805FD598) {
            *(f32*)(p + 0x1C0) = cooldown;
            for (u32 i = 0; i < 5u; i++) {
                u8* obj = *(u8**)(p + 0x1D4 + i * 8);
                if (obj != NULL) {
                    *(u8**)(p + 0x1D4 + i * 8) = NULL;
                    ReleaseContact(p, obj);
                }
            }
        }
    }
    if (*(u16*)(p + 0x100) != 0) ZoneEffectsUpdate(p + 0xFC, dt);
}

// Makes sure a camera is current: with no camera, pulls the matrix from the level object (virtual
// slot 0x38), applies it to the main camera and returns the current camera global.
static u8* ResolveCamera(u8* cam) {
    if (cam == NULL) {
        f32 mtx[3][4];
        u8* obj = *(u8**)(lbl_803C4298 + 0x4);
        (*(void (**)(u8*, f32(*)[4]))(*(u8**)obj + 0x38))(obj, mtx);
        CameraSetMatrix(lbl_8047C640, mtx);
        fn_80009840(lbl_803C4298);
        cam = (u8*)lbl_805FB5F8;
    }
    return cam;
}

// Bilinear blend of the camera's two view-plane rectangles (+0x120 near, +0x150 far; four corners
// of 3 floats each) at the screen position (x, y); gives the ray's near and far points.
static void ScreenRay(u8* cam, f32 x, f32 y, f32* nr, f32* fr) {
    f32 W = (f32)gScreenWidth;
    f32 H = (f32)gScreenHeight;
    f32 one = lbl_805FD5F0;
    f32 u = x / W;
    f32 v = one - (y / H);
    f32 a = (one - u) * v;
    f32 b = u * v;
    f32 w = one - v;
    f32 c = u * w;
    f32 d = (one - u) * w;
#define M(off) (*(f32*)(cam + (off)))
    nr[0] = ((M(0x12C) * b + M(0x120) * a) + M(0x138) * c) + M(0x144) * d;
    nr[1] = ((M(0x130) * b + M(0x124) * a) + M(0x13C) * c) + M(0x148) * d;
    nr[2] = ((M(0x134) * b + M(0x128) * a) + M(0x140) * c) + M(0x14C) * d;
    fr[0] = ((M(0x15C) * b + M(0x150) * a) + M(0x168) * c) + M(0x174) * d;
    fr[1] = ((M(0x160) * b + M(0x154) * a) + M(0x16C) * c) + M(0x178) * d;
    fr[2] = ((M(0x164) * b + M(0x158) * a) + M(0x170) * c) + M(0x17C) * d;
#undef M
}

// 0x80073B68
// Screen point (x, y) to world: writes the near and far ray points (when the pointers are given)
// and the point at fraction t along the ray to out.
void fn_80073B68(f32* out, f64 t, f32 x, f32 y, u8* cam, f32* nearOut, f32* farOut) {
    f32 nr[3];
    f32 fr[3];
    cam = ResolveCamera(cam);
    ScreenRay(cam, x, y, nr, fr);
    if (nearOut != NULL) {
        nearOut[0] = nr[0];
        nearOut[1] = nr[1];
        nearOut[2] = nr[2];
    }
    if (farOut != NULL) {
        farOut[0] = fr[0];
        farOut[1] = fr[1];
        farOut[2] = fr[2];
    }
    f32 tt = (f32)t;
    f32 dx = fr[0] - nr[0];
    f32 dy = fr[1] - nr[1];
    f32 dz = fr[2] - nr[2];
    f32 sx = dx * tt;
    f32 sy = dy * tt;
    f32 sz = dz * tt;
    out[0] = nr[0] + sx;
    out[1] = nr[1] + sy;
    out[2] = nr[2] + sz;
}

// 0x80073F58
// Like fn_80073B68, but the fraction t is derived from the world point `pos` by fn_800734B4.
void fn_80073F58(f32* out, u8* cam, f32* pos, f32* nearOut, f32* farOut, f32 x, f32 y) {
    f32 nr[3];
    f32 fr[3];
    f32 vec[3];
    cam = ResolveCamera(cam);
    vec[0] = pos[0];
    vec[1] = pos[1];
    vec[2] = pos[2];
    f64 t = fn_800734B4(cam, vec);
    cam = ResolveCamera(cam);
    ScreenRay(cam, x, y, nr, fr);
    if (nearOut != NULL) {
        nearOut[0] = nr[0];
        nearOut[1] = nr[1];
        nearOut[2] = nr[2];
    }
    if (farOut != NULL) {
        farOut[0] = fr[0];
        farOut[1] = fr[1];
        farOut[2] = fr[2];
    }
    f32 tt = (f32)t;
    f32 dx = fr[0] - nr[0];
    f32 dy = fr[1] - nr[1];
    f32 dz = fr[2] - nr[2];
    f32 sx = dx * tt;
    f32 sy = dy * tt;
    f32 sz = dz * tt;
    out[0] = nr[0] + sx;
    out[1] = nr[1] + sy;
    out[2] = nr[2] + sz;
}

// Number of slots (pointer array at +0x9C, count at +0x1B4) whose object has a leak level > 0.
static s32 CountLeaking(u8* p) {
    s32 n = *(s32*)(p + 0x1B4);
    s32 cnt = 0;
    for (s32 i = 0; i < n; i++) {
        u8* slot = *(u8**)(p + 0x9C + i * 4);
        if (slot != NULL && *(u8*)(slot + 0x26C) > 0) cnt++;
    }
    return cnt;
}

// 0x80095BEC
// Leak-plate controller: when enough time has passed (interpolated from the difficulty tables) a
// random eligible slot (can leak, not leaking yet) starts leaking with a level-dependent sound;
// starts/stops the rattle loop when the first slot starts/the last slot stops leaking.
void fn_80095BEC(u8* p, f32 dt) {
    s32 before = CountLeaking(p);
    *(f32*)(p + 0x364) = *(f32*)(p + 0x364) + dt;
    s32 now = CountLeaking(p);

    bool fire;
    if (now >= lbl_80347180[RaySceneTest(NULL)]) {  // r3 leftover: RaySceneTest takes no argument
        fire = false;
    } else {
        f32 k = lbl_805FDB64 - (*(f32*)(p + 0x358) / lbl_805FDB3C);
        if (lbl_80347090[RaySceneTest(NULL)] == 2) k = k * k;
        f32 lo = lbl_803470B0[RaySceneTest(NULL)];
        f32 lo2 = k * (lbl_803470D0[RaySceneTest(NULL)] - lo);
        f32 minT = lbl_803470B0[RaySceneTest(NULL)] + lo2;
        f32 hi = lbl_803470C0[RaySceneTest(NULL)];
        f32 hi2 = k * (lbl_803470E0[RaySceneTest(NULL)] - hi);
        fire = *(f32*)(p + 0x364) >= minT + *(f32*)(p + 0x368) * ((lbl_803470C0[RaySceneTest(NULL)] + hi2) - minT);
    }

    if (fire) {
        RandomU32();
        s32 n = *(s32*)(p + 0x1B4);
        u32 eligible = 0;
        for (s32 i = 0; i < n; i++) {
            u8* slot = *(u8**)(p + 0x9C + i * 4);
            if (*(u8*)(slot + 0x26E) != 0 && *(u8*)(slot + 0x26C) == 0) eligible++;
        }
        u32 pick = RandomU32();
        // the original divides unchecked (divwu); with no eligible slot the pick is never used
        pick = (eligible != 0) ? (pick % eligible) : 0;
        n = *(s32*)(p + 0x1B4);
        for (s32 i = 0; i < n; i++) {
            u8* slot = *(u8**)(p + 0x9C + i * 4);
            if (*(u8*)(slot + 0x26E) != 0 && *(u8*)(slot + 0x26C) == 0) {
                if (pick == 0) {
                    s32 level = 0;
                    switch (fn_800972CC(p)) {
                        case 0:
                            level = lbl_80347150[RaySceneTest(NULL)];
                            PlayExperimentSound(NULL, kSndLeak1, 1);
                            break;
                        case 1:
                            level = lbl_80347160[RaySceneTest(NULL)];
                            PlayExperimentSound(NULL, kSndLeak2, 1);
                            break;
                        case 2:
                            level = lbl_80347170[RaySceneTest(NULL)];
                            PlayExperimentSound(NULL, kSndLeak3, 1);
                            break;
                        default:
                            break;
                    }
                    fn_800980F8(*(u8**)(p + i * 4 + 0x9C), level);
                    break;
                }
                pick--;
            }
        }
        *(f32*)(p + 0x364) = lbl_805FDB28;
        *(f32*)(p + 0x368) = lbl_805FDB2C * (f32)RandomU32();
    }

    if (before == 0) {
        if (CountLeaking(p) > 0) PlayExperimentSound(NULL, kSndRattle, 1);
    } else if (before > 0) {
        if (CountLeaking(p) == 0) StopExperimentSound(NULL, kSndRattle);
    }
}

// 0x80096038
// Picks, for each of 8 direction buckets (fn_80097064 on the slot's direction), the slot object
// whose direction is closest in angle to the reference axis (0, 1, 0) and records the highest
// level per bucket; then advances a timer and lights up 8 ring entries with a ramped colour.
void fn_80096038(u8* p, f32 dt) {
    f32 zero = lbl_805FDB28;
    f32 one = lbl_805FDB64;
    s32 best[8];
    f32 ang[8];
    f32 refx = zero;
    f32 refy = one;
    f32 refz = zero;

    for (s32 i = 0; i < 8; i++) {
        ang[i] = lbl_805FDB3C;
        *(s32*)(p + 0x1B8 + i * 4) = 0;
        best[i] = 0;
    }

    s32 count = *(s32*)(p + 0x1B4);
    for (s32 i = 0; i < count; i++) {
        u8* slot = *(u8**)(p + 0x9C + i * 4);
        if (*(u8*)(slot + 0x26E) != 0 && slot != *(u8**)(p + 0x1D8)) {
            f32 v[3];
            f32 lenV;
            fn_80097BCC((u8*)v, slot);
            f32 l2 = (v[2] * v[2]) + ((v[0] * v[0]) + (v[1] * v[1]));
            if (l2 <= 0.0f) lenV = 0.0f;
            else lenV = sqrtf(l2);

            f32 lenR;
            f32 r2 = (refz * refz) + ((refx * refx) + (refy * refy));
            if (r2 <= 0.0f) lenR = 0.0f;
            else lenR = sqrtf(r2);

            f32 dot = (v[2] * refz) + ((v[0] * refx) + (v[1] * refy));
            f32 denom = lenV * lenR;
            f32 a;
            if (denom <= lbl_805FDB28) {
                a = lbl_805FDB28;
            } else {
                f32 c = dot / denom;
                if (c < lbl_805FDB38) c = lbl_805FDB38;
                else if (c > lbl_805FDB64) c = lbl_805FDB64;
                f32 c2 = c;
                if (c2 < lbl_805FDB38) c2 = lbl_805FDB38;
                else if (c2 > lbl_805FDB64) c2 = lbl_805FDB64;
                a = lbl_805FDB84 * (f32)fn_8033F1F4((f64)c2);
            }

            f32 dir[3];
            dir[0] = v[0];
            dir[1] = v[1];
            dir[2] = v[2];
            s32 b = fn_80097064(dir);
            s32 level = *(u8*)(slot + 0x26C);
            if (level > best[b]) best[b] = level;
            if (a < ang[b]) {
                ang[b] = a;
                *(u8**)(p + 0x1B8 + b * 4) = slot;
            }
        }
    }

    f32 tm = *(f32*)(p + 0x378) + dt;
    *(f32*)(p + 0x378) = tm;
    f32 step = (one / (one + (lbl_805FDB88 * (one - (*(f32*)(p + 0x358) / lbl_805FDB3C))))) * lbl_805FDB74;
    s32 n = (s32)(f32)floor((f64)(tm / step));
    if (n > 0) {
        s32 head = (*(s32*)(p + 0x37C) + n) % 8;
        *(f32*)(p + 0x378) = lbl_805FDB28;
        *(s32*)(p + 0x37C) = head;
        for (s32 i = 0; i < 8; i++) {
            f32 s = (f32)i / lbl_805FDB8C;
            f32 col[4];
            col[0] = one * s;
            col[1] = col[0];
            col[2] = zero * s;
            col[3] = one;
            s32 ring = (i + *(s32*)(p + 0x37C) + 1) % 8;
            fn_8009875C(*(u8**)(p + ring * 4 + 0x194), col);
        }
    }
}

}  // extern "C"
