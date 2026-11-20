// Missing functions batch 14: catch mini-game updates, animated-prop influence blending by prop,
// camera shake and frame timing, brazier wand status, ground material lookup, misc callbacks.
#include <stddef.h>
#include <stdint.h>

#include "types.h"

extern "C" {
#include "game/script_callbacks.h"

void*  EventPost(s32 id, void* sender);                              // 0x801F3918
BOOL   FighterStatsHasEffect(u8* stats, s32 id);                     // 0x80214050
int    SysSprintf(char* buf, const char* fmt, ...);                  // 0x80142EFC
s32 TriggerDoAction(void* action, void* a1, void* a2);            // 0x801E0054
void   UISetText(void* el, const char* text, s32 a);                 // 0x8019EFC8
void   UIWidgetPlayAnimLoop(void* w, u32 anim, s32 n);               // 0x8019BB1C
BOOL   ZoneIsLoaded(void* world, s32 zone);                          // 0x8012F408
f32    __cvt_ull_flt(u64 v);                                         // 0x8032FA98 (u64 -> float)
void   CatchObject_Release(void* obj, s32 a);                        // 0x80030A98
void   fn_80021CAC(f32 farg0);                                       // 0x80021CAC
void   fn_8002FE14(u8* g, s32 kind);                                 // 0x8002FE14
s32    fn_8008D254(u8* a3);                                          // 0x8008D254
void   fn_800E4E80(u8* p, s32 clip);                                 // 0x800E4E80
void   fn_800E4F84(u8* p);                                           // 0x800E4F84
u64    fn_801425DC(u8* timer);                                       // 0x801425DC
f32    fn_8018BE30(void* obj, f32 a);                                // 0x8018BE30
s32    fn_8019BC80(u8* arg0, u32 arg1);                              // 0x8019BC80
void*  fn_801B7198(void* anim, s32 a);                               // 0x801B7198
s32    fn_801ED538(u8* a3);                                          // 0x801ED538
u8*    fn_801ED6E4(u32 a0);                                          // 0x801ED6E4
BOOL   fn_801F5220(void);                                            // 0x801F5220
void   fn_801F5714(s32 flags);                                       // 0x801F5714
void   fn_801F5814(s32 a);                                           // 0x801F5814
u8     fn_8021EFE8(u8* a3);                                          // 0x8021EFE8
u32    RandomU32(void);

extern u8  gGame[];                  // 0x803B53A0
extern u8  gWorldState[];            // 0x803E6D00 (holds a pointer to the world block)
extern u8* gBattle;                  // 0x805FBBB8
extern "C++" s32 gZoneResourceSize;  // 0x805FB94C
extern u8  lbl_803C4298[];           // 0x803C4298
extern u8  lbl_805FB768[];           // 0x805FB768 (timer object)
extern u8* lbl_805FAF78;             // 0x805FAF78 (lightning manager)
extern s32 lbl_805FAF94;             // 0x805FAF94 (countdown)
extern u8  lbl_805FAF99;             // 0x805FAF99
extern u32 lbl_805FB0A0;             // 0x805FB0A0 (last disabled HUD input hash)
extern s32 lbl_805FB6B8;             // 0x805FB6B8
extern u8  lbl_805F9618;             // 0x805F9618 (timer needs init)
extern u32 lbl_805FB838;             // 0x805FB838 (frame counter, high word)
extern u32 lbl_805FB83C;             // 0x805FB83C (frame counter, low word)
extern f32 lbl_805FB840;             // 0x805FB840 (frames per second)
extern f32 lbl_805FB844;             // 0x805FB844 (minimum fps over the last 30 samples)
extern u32 lbl_805FB848;             // 0x805FB848 (last sample time, high word)
extern u32 lbl_805FB84C;             // 0x805FB84C (last sample time, low word)
extern s32 lbl_805FB850;             // 0x805FB850 (sample count)
extern u8* lbl_805FBBE0;             // 0x805FBBE0 (save block pointer)
}

static const u32 kHashAll = 0xADBF3357u;      // "ALL"
static const u32 kHashActions = 0x3A0D0FF5u;  // "Actions"

typedef void (*VFn)(void* self, f32 a);

// Finds the animated prop (object type 0xF) whose handle matches `prop`, searching every loaded
// zone. Returns NULL when there is none.
static u8* FindAnimatedPropByProp(u32 prop) {
    u8* world = *(u8**)gWorldState;
    for (s32 zone = 0; zone <= *(s32*)(world + 0x4); zone++) {
        if (!ZoneIsLoaded(gWorldState, zone)) continue;
        u8* res = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
        for (s32 i = 0; i < *(u8*)(res + 0xAC); i++) {
            u8* obj = *(u8**)(*(u8**)(res + 0xA8) + i * 4);
            if (*(s32*)(obj + 0x8) == 0xF) {
                s32 h = fn_801ED538(obj + 0x58);
                if (fn_801B7198(&h, 0) == (void*)(uintptr_t)prop) return obj;
            }
        }
    }
    return NULL;
}

// Blend step shared by the by-id and by-prop variants: reads the blend position from the prop's
// animation (virtual slot 0x10, then fn_8018BE30); at 1.0 the blend ends, otherwise the two
// influences are set to 2*t and 2*(1-t).
static void UpdateInfluenceBlend(u8* p) {
    if (*(u8**)(p + 0x78) == NULL) return;
    s32 h = fn_801ED538(p + 0x58);
    u8* anim = (u8*)fn_801B7198(&h, 0);
    void* obj = (*(void* (**)(void*))(*(u8**)anim + 0x10))(anim);
    f32 t = fn_8018BE30(obj, 0.0f);
    if (t == 1.0f) {
        *(u8*)(*(u8**)(p + 0x78) + 0x54) = 0;
        *(u8**)(p + 0x78) = NULL;
        *(f32*)(p + 0x44) = 1.0f;
        return;
    }
    *(f32*)(p + 0x44) = 2.0f * t;
    *(f32*)(*(u8**)(p + 0x78) + 0x44) = 2.0f * (1.0f - t);
}

// Shared by CameraShake and CameraShakeSynced.
static void CameraShakeImpl(u8* src, f32 a1, f32 a2, f32 a3) {
    if (!fn_801F5220()) return;
    u8* pos = *(u8**)(src + 0x4);
    u8* cam = *(u8**)(*(u8**)(gGame + 0xD614) + 0x4);
    f32 dx = *(f32*)(pos + 0x0) - *(f32*)(cam + 0x0);
    f32 dy = *(f32*)(pos + 0x4) - *(f32*)(cam + 0x4);
    f32 dz = *(f32*)(pos + 0x8) - *(f32*)(cam + 0x8);
    if (!(dz * dz + (dx * dx + dy * dy) < 400.0f)) return;
    if (*(u32*)(cam + 0x20) & 0x04000000) {
        u8* ev = (u8*)EventPost(0x25, src);
        if (ev != NULL) {
            *(f32*)(ev + 0x4) = a1;
            *(f32*)(ev + 0x8) = a2;
            *(f32*)(ev + 0xC) = a3;
        }
    } else {
        u8* target = *(u8**)(lbl_803C4298 + 0x4);
        (*(VFn*)(*(u8**)target + 0xA0))(target, a1);
        (*(VFn*)(*(u8**)target + 0xA8))(target, a2);
        (*(VFn*)(*(u8**)target + 0xA4))(target, a3);
    }
}

extern "C" {

// 0x8001DE30
// True when all five slots of the lightning manager hold a valid entry and the current selection
// of the second list is not an effect that blocks charging (fn_8021EFE8).
s32 CanChargeMonster(void) {
    u8* mgr = lbl_805FAF78;
    s32 allValid;
    if (mgr != NULL && *(s32*)mgr != 0) {
        u8* save = lbl_805FBBE0;
        u8* q = mgr + 8;
        allValid = 1;
        for (s32 n = 5; n > 0; n--, q += 0x1A4) {
            s32 x = -1;
            if (*(s32*)(q + 0x198) > 0) {
                s32 idx = *(s32*)(q + 0x190);
                if (idx >= 0) {
                    s32 v = *(s32*)(q + idx * 4);
                    if (v >= 0) x = **(s32**)(save + v * 4 + 0xA58);
                }
            }
            if (x == -1) {
                allValid = 0;
                break;
            }
        }
    } else {
        allValid = 0;
    }
    if (allValid == 0) return 0;

    s32 blocked = 0;
    if (mgr != NULL && *(s32*)mgr != 0) {
        s32 cnt = *(s32*)(mgr + 0x830);
        u8* list = mgr + 0x698;
        s32 v = -1;
        if (cnt > 0 && *(s32*)(list + 0x190) < cnt) v = *(s32*)(list + *(s32*)(list + 0x190) * 4);
        if (v >= 0) {
            if (fn_8021EFE8(*(u8**)(lbl_805FBBE0 + v * 4 + 0xA58)) != 0) blocked = 1;
        }
    }
    if (blocked == 1) return 0;
    return 1;
}

// 0x80048AB8
// Whether an action of kind a2 and cost a0 can be afforded with a1 points. Kind 5 needs the second
// fighter's flag at +0x19 and single player; kind 3 is refused while effect 0xE is active.
// Some HUD input modes then restrict the kinds further.
s32 CanAffordAction(s32 a0, s32 a1, s32 a2) {
    if (a0 > a1) return 0;
    if (a2 == 5) {
        if (*(u8*)(*(u8**)(gBattle + 0xA04) + 0x19) == 0 || IsMultiPlayer() != 0) return 0;
    }
    if (FighterStatsHasEffect(*(u8**)(gBattle + 0xA00) + 0x20, 0xE) != 0 && a2 == 3) return 0;
    switch (lbl_805FB0A0) {
    case kHashActions:
    case kHashAll:
        return 1;
    case 0x24E68B02u:  // hash of an unknown name
        return a2 == 0;
    case 0x7B3219A5u:  // hash of an unknown name
        return (u32)(a2 - 2) <= 1;
    case 0xAD4C5703u:  // hash of an unknown name
        return 1;
    default:
        return 1;
    }
}

// 0x8005A420
// Takes the character off the ground: triggers the ground object's leave actions (once when its
// +0x8C is clear it also sets the entity's airborne flags), then clears the ground link and the
// velocity block at +0x7B8..0x7CC, with +0x7D0 reset to 1.0.
void CharacterLeaveGround(u8* arg0) {
    s32 handle = *(s32*)(arg0 + 0x7B0);
    u8* g = (u8*)fn_801B7198(&handle, 0);
    if (g != NULL) {
        if (*(s32*)(g + 0x8C) == 0) {
            *(s32*)(arg0 + 0x790) = 1;
            *(u32*)(arg0 + 0x838) |= 0x02000000;
        }
        u8* sub = *(u8**)(g + 0x9C);
        if (sub != NULL && ((*(u32*)(sub + 0xA4) >> 27) & 0xF)) TriggerDoAction(sub + 0xA4, arg0, g);
        if ((*(u32*)(g + 0x74) >> 27) & 0xF) TriggerDoAction(g + 0x74, arg0, g);
    }
    *(s32*)(arg0 + 0x7B4) = 0;
    *(s32*)(arg0 + 0x7B0) = lbl_805FB6B8;
    *(f32*)(arg0 + 0x7B8) = 0.0f;
    *(f32*)(arg0 + 0x7BC) = 0.0f;
    *(f32*)(arg0 + 0x7C0) = 0.0f;
    *(f32*)(arg0 + 0x7C4) = 0.0f;
    *(f32*)(arg0 + 0x7C8) = 0.0f;
    *(f32*)(arg0 + 0x7CC) = 0.0f;
    *(f32*)(arg0 + 0x7D0) = 1.0f;
}

// 0x800224C4
// Counts the base timer (+0x6C) down by dt (previous value kept at +0x74) while it is not negative:
// refreshes the shown whole-second text, and below 6.0 starts the final countdown (+0x82).
void CatchMinigame_UpdateBase(void* g, f32 dt) {
    u8* a = (u8*)g;
    f32 prev = *(f32*)(a + 0x6C);
    f32 cur = prev - dt;
    *(f32*)(a + 0x74) = prev;
    *(f32*)(a + 0x6C) = cur;
    if (cur >= 0.0f) {
        if (*(u8**)(a + 0x68) != NULL) {
            s32 shownBefore = (s32)prev;
            s32 shownNow = (s32)cur;
            if (shownBefore != shownNow) {
                char buf[32];
                SysSprintf(buf, "%d", shownNow);
                UISetText(*(void**)(a + 0x68), buf, 1);
            }
        }
        if (*(u8*)(a + 0x82) == 0) {
            f32 t = *(f32*)(a + 0x6C);
            if (t < 6.0f) {
                *(u8*)(a + 0x82) = 1;
                s32 secs = (s32)t;
                lbl_805FAF94 = secs;
                if (secs >= 5) lbl_805FAF99 = 1;
            }
        }
        if (*(u8*)(a + 0x82) != 0) fn_80021CAC(*(f32*)(a + 0x6C));
    }
}

// 0x8002EF4C
// Updates every object of a lane (list at lane+0xC, next at +0xC): objects that fell below 2.0
// are released; while the grabber is open (+0x2A4 == 1) and no object was hit yet, the first
// object overlapping the grabber position is caught (kind 1 closes the grabber and picks it up,
// other kinds bump it with a rumble) and its slot in the object table (+0xEE4) is freed.
void CatchMinigame_UpdateLane(void* g, void* lane, BOOL active, f32 dt) {
    u8* a = (u8*)g;
    typedef s32 (*HitFn)(void* self, f32* pos);
    typedef void (*VoidFn)(void* self);
    typedef void (*ShowFn)(void* self, s32 mode);
    s32 hit = 0;
    u8* node = *(u8**)((u8*)lane + 0xC);
    while (node != NULL) {
        u8* next = *(u8**)(node + 0xC);
        (*(VFn*)(*(u8**)node + 0x10))(node, dt);
        if (*(f32*)(*(u8**)(node + 0x4) + 0x68) < 2.0f) {
            (*(VoidFn*)(*(u8**)node + 0x14))(node);
        } else if (*(s32*)(a + 0x2A4) == 1 && active != 0 && hit == 0 && *(s32*)(node + 0x1C) != 5) {
            u8* grab = *(u8**)(a + 0x27C);
            f32 pos[3];
            pos[0] = *(f32*)(grab + 0x68);
            pos[1] = *(f32*)(grab + 0x6C);
            pos[2] = *(f32*)(grab + 0x70);
            hit = (*(HitFn*)(*(u8**)node + 0xC))(node, pos);
            if (hit != 0) {
                if (*(s32*)(node + 0x1C) == 1) {
                    *(s32*)(a + 0x2A4) = 2;
                    (*(VoidFn*)(*(u8**)(a + 0x4) + 0x64))(a);
                    *(u8**)(a + 0xF0C) = node;
                    if (*(s32*)(*(u8**)(a + 0x27C) + 0x10) == 0) {
                        // grabber state is 2 here, so the "closed grabber" texture is chosen
                        UIImage_SetTexture((u32)(uintptr_t) * (void**)(*(u8**)(a + 0x27C)),  // lint:ok header takes u32
                                           (BOOL)UITextureList_GetTexture(0x7AD719E2u, 3));
                    }
                    u8* effect = *(u8**)(a + 0x2A0);
                    if (effect != NULL) {
                        *(f32*)(effect + 0x68) = *(f32*)(*(u8**)(a + 0x27C) + 0x68);
                        *(f32*)(*(u8**)(a + 0x2A0) + 0x6C) = *(f32*)(*(u8**)(a + 0x27C) + 0x6C);
                        u8* e2 = *(u8**)(a + 0x2A0);
                        (*(ShowFn*)(*(u8**)(e2 + 0x138) + 0x2C))(e2, 0x1A);
                    }
                } else {
                    u8* gr = *(u8**)(a + 0x27C);
                    if (*(s32*)(gr + 0x10) != 0) fn_8019BC80(gr, 0);
                    *(s32*)(a + 0x2A8) = 0;
                    UIWidgetPlayAnimLoop(*(void**)(a + 0x27C), 0xA8D5FBDFu, 10);
                    fn_8002FE14(a, *(s32*)(node + 0x1C));
                    TimedRumble(*(s32*)(lbl_803C4298 + 0x38), 0.1f, 4);
                }
                (*(VoidFn*)(*(u8**)node + 0x14))(node);
                s32 slot = *(s32*)(node + 0x10);
                if (slot > -1 && slot < 10) {
                    u8* obj = *(u8**)(a + slot * 4 + 0xEE4);
                    if (obj != NULL) {
                        (*(VoidFn*)(*(u8**)obj + 0x14))(obj);
                        CatchObject_Release(obj, 0);
                    }
                    *(s32*)(node + 0x10) = -1;
                }
            }
        }
        node = next;
    }
}

// 0x8002FFBC
// Scrolls the two background strips left (speeds at +0x2AC/+0x2B0) and moves them on the widgets;
// when a strip has scrolled a full 640 width off-screen it wraps by 640 and swaps its list entries.
void CatchMinigame_UpdateObjects(void* g, f32 dt) {
    u8* a = (u8*)g;
    typedef void (*ShowFn)(void* self, s32 mode);
    for (s32 i = 0; i < 2; i++) {
        u8* w1 = *(u8**)(a + i * 4 + 0x284);
        *(f32*)(w1 + 0x68) = *(f32*)(w1 + 0x68) - dt * *(f32*)(a + 0x2AC);
        w1 = *(u8**)(a + i * 4 + 0x284);
        (*(ShowFn*)(*(u8**)(w1 + 0x138) + 0x2C))(w1, 0x18);
        u8* w2 = *(u8**)(a + i * 4 + 0x28C);
        *(f32*)(w2 + 0x68) = *(f32*)(w2 + 0x68) - dt * *(f32*)(a + 0x2B0);
        w2 = *(u8**)(a + i * 4 + 0x28C);
        (*(ShowFn*)(*(u8**)(w2 + 0x138) + 0x2C))(w2, 0x18);
    }
    u8* s1 = *(u8**)(a + 0x284);
    f32 x1 = 640.0f + *(f32*)(s1 + 0x68);
    if (!(x1 >= 0.0f)) {
        *(f32*)(s1 + 0x68) = 640.0f + x1;
        *(u8**)(a + 0x284) = *(u8**)(a + 0x288);
        *(u8**)(a + 0x288) = s1;
    }
    u8* s2 = *(u8**)(a + 0x28C);
    f32 x2 = 640.0f + *(f32*)(s2 + 0x68);
    if (!(x2 >= 0.0f)) {
        *(f32*)(s2 + 0x68) = 640.0f + x2;
        *(u8**)(a + 0x28C) = *(u8**)(a + 0x290);
        *(u8**)(a + 0x290) = s2;
    }
}

// 0x80030414
// Puts a catch object on the lane start (x, y from target, z = 10.0), shows it, picks a random
// variant (0..2) at +0x24, activates it (+0x20), plays widget mode 0x1A and calls virtual slot 0x1C
// with the speed.
void CatchObject_Launch(void* obj, const f32 target[3], f32 speed) {
    u8* o = (u8*)obj;
    typedef void (*ShowFn)(void* self, s32 mode);
    typedef void (*LaunchFn)(void* self, f32 speed);
    u8* widget = *(u8**)(o + 0x4);
    *(f32*)(widget + 0x68) = target[0];
    *(f32*)(widget + 0x6C) = target[1];
    *(f32*)(widget + 0x70) = 10.0f;
    fn_8019BC80(*(u8**)(o + 0x4), 0);
    u32 r = RandomU32();
    *(u8*)(o + 0x20) = 1;
    *(s32*)(o + 0x24) = (s32)(r % 3);
    widget = *(u8**)(o + 0x4);
    (*(ShowFn*)(*(u8**)(widget + 0x138) + 0x2C))(widget, 0x1A);
    (*(LaunchFn*)(*(u8**)o + 0x1C))(o, speed);
}

// 0x8007472C
// Maps an experiment type (0..11) to its castle access id; -1 when out of range.
u32 /*0x4F*/ CastleAccessFromExperimentType(u32 /*0x4D*/ a0) {
    switch (a0) {
    case 0:  return 0xB;
    case 1:  return 0x9;
    case 2:  return 0x7;
    case 3:  return 0x6;
    case 4:  return 0x8;
    case 5:  return 0xF;
    case 6:  return 0x11;
    case 7:  return 0x10;
    case 8:  return 0xC;
    case 9:  return 0xD;
    case 10: return 0xE;
    case 11: return 0xA;
    default: return (u32)-1;
    }
}

// 0x8008C900
// Status of the brazier wand: -1 without a wand object (+0x378), 0 when it is busy (state 1),
// 3 when the owner state (+0x35C) is 0x32, 2 when the wand's target (+0x268) or the other
// target (+0x374) differs from the owner (+0x370), else 1.
s32 BrazierWandGetWandStatus(void* a0) {
    u8* p = (u8*)a0;
    if (*(u8**)(p + 0x378) == NULL) return -1;
    if (fn_8008D254(*(u8**)(p + 0x378)) == 1) return 0;
    if (fn_8008D254(*(u8**)(p + 0x378)) != 0) return -1;
    if (*(s32*)(p + 0x35C) == 0x32) return 3;
    s32 target = *(s32*)(*(u8**)(p + 0x378) + 0x268);
    if (target != 0 && target != *(s32*)(p + 0x370)) return 2;
    s32 other = *(s32*)(p + 0x374);
    if (other != 0 && other != *(s32*)(p + 0x370)) return 2;
    return 1;
}

// 0x800E5154
// Enables clip a1 on the animated prop whose handle is a0 (searched through the loaded zones).
void CIAnimatedPropEnableByProp(u32 /*0x31*/ a0, u32 /*0x2B*/ a1) {
    u8* p = FindAnimatedPropByProp(a0);
    fn_800E4E80(p, (s32)a1);
}

// 0x800E5238
// Starts the influence blend on the animated prop with entity id a0.
void CIAnimatedPropStartInfluenceBlend(u32 /*0x17*/ a0) {
    fn_800E4F84(fn_801ED6E4(a0));
}

// 0x800E525C
// Starts the influence blend on the animated prop whose handle is a0.
void CIAnimatedPropStartInfluenceBlendByProp(u32 /*0x31*/ a0) {
    fn_800E4F84(FindAnimatedPropByProp(a0));
}

// 0x800E5338
// Per-frame influence blend update of the animated prop with entity id a0.
void CIAnimatedPropUpdateInfluenceBlend(u32 /*0x17*/ a0) {
    UpdateInfluenceBlend(fn_801ED6E4(a0));
}

// 0x800E53E0
// Per-frame influence blend update of the animated prop whose handle is a0.
void CIAnimatedPropUpdateInfluenceBlendByProp(u32 /*0x31*/ a0) {
    UpdateInfluenceBlend(FindAnimatedPropByProp(a0));
}

// 0x800E6188
// Flags the panner (+0xA4) to return to zero.
void CIPannerReturnToZero(u32 /*0x17*/ a0) {
    *(u8*)(fn_801ED6E4(a0) + 0xA4) = 1;
}

// 0x800E7884
// Shakes the camera when the source is within 20 units of the camera target: through an event
// (0x25) carrying the three amounts when the target has flag 0x04000000, otherwise directly
// through the camera object's virtual slots 0xA0, 0xA8 and 0xA4.
void CameraShake(void* a0, f32 a1, f32 a2, f32 a3) {
    CameraShakeImpl((u8*)a0, a1, a2, a3);
}

// 0x800E79DC
// Identical to CameraShake in the original code.
void CameraShakeSynced(void* a0, f32 a1, f32 a2, f32 a3) {
    CameraShakeImpl((u8*)a0, a1, a2, a3);
}

// 0x800EF8B8
// Returns the hash of the ground material below the entity, from the 4-bit index at
// +0x748 (bits 12..15): goo, grass, water, wood, 0xB5CE36F5, metal, tile, dirt, 0x1A811CBE;
// anything else is "default".
u32 CharacterGetGroundMatBelow(u32 /*0x2E*/ a0) {
    u8* p = (u8*)(uintptr_t)a0;
    switch ((*(u32*)(p + 0x748) >> 12) & 0xF) {
    case 1:  return 0x1B164D9Cu;  // "goo"
    case 2:  return 0x5F595D43u;  // "grass"
    case 3:  return 0x0C67D66Au;  // "water"
    case 4:  return 0x0A08F4B9u;  // "wood"
    case 5:  return 0xB5CE36F5u;
    case 6:  return 0x37363679u;  // "metal"
    case 7:  return 0x401A44B0u;  // "tile"
    case 8:  return 0xAEF20C02u;  // "dirt"
    case 9:  return 0x1A811CBEu;
    default: return 0xDC1BFEC5u;  // "default"
    }
}

// 0x8013FE28
// Cancels the current battle: fn_801F5714(1), posts event 0x4E, then fn_801F5814(1).
void CancelBattle(void) {
    fn_801F5714(1);
    EventPost(0x4E, NULL);
    fn_801F5814(1);
}

// 0x801D8C04
// Frame-rate sampler: once more than 100000 timer ticks passed since the last sample, computes
// fps = 1000000 * frames / elapsed (elapsed clamped to 0x7FFFFFFF), keeps the minimum over 30
// samples in lbl_805FB844, and restarts the frame counter.
void CameraUpdate(void* camera) {
    (void)camera;  // the original ignores its argument
    if (lbl_805F9618 != 0) {
        u64 t = fn_801425DC(lbl_805FB768);
        lbl_805FB84C = (u32)t;
        lbl_805FB848 = (u32)(t >> 32);
        lbl_805F9618 = 0;
    }
    u64 now = fn_801425DC(lbl_805FB768);
    u64 last = ((u64)lbl_805FB848 << 32) | lbl_805FB84C;
    if (last + 100000 < now) {
        u64 delta = now - last;
        if (delta > 0x7FFFFFFF) delta = 0x7FFFFFFF;
        f32 fps = (1000000.0f * (f32)(s32)lbl_805FB83C) / __cvt_ull_flt(delta);
        lbl_805FB840 = fps;
        if (lbl_805FB850 < 0x1E) {
            f32 m = lbl_805FB844;
            if (m >= fps) m = fps;
            lbl_805FB844 = m;
            lbl_805FB850 += 1;
        } else {
            lbl_805FB844 = fps;
            lbl_805FB850 = 0;
        }
        lbl_805FB84C = (u32)now;
        lbl_805FB83C = 0;
        lbl_805FB838 = 0;
        lbl_805FB848 = (u32)(now >> 32);
    }
}

}  // extern "C"
