// Part action script callbacks (PartAction1/2 GetCost/GetValue/GetIsTarget), part-slot highlight
// update, entity/camera helpers, nunchuk input, heap memalign and the multiplayer init, verified
// against the asm (0x8001D4FC - 0x801EF6E8).
#include <math.h>

#include "types.h"
#include "game/part.h"
#include "game/character.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" void UpdateAllZoneEntities(void);  // 0x80010228 (C++ linkage, defined in game_flow.cpp)

typedef void (*CamModeFn)(void*, s32);            // camera vtable + 0xC
typedef void (*CamLookFn)(void*, f32*, f32*);     // camera vtable + 0x2C
typedef void (*CamBlendFn)(void*, f32);           // camera vtable + 0x30
typedef s32 (*TreeVisitFn)(void*, void*);         // widget tree callback

// Limb list filled by TargetListDecode: count sits at +0x14 (same layout as part_instance.cpp).
struct TargetListBuf {
    s32 limbs[5];
    s32 count;
};

extern "C" {
void*  CharacterFind(s32 id);                                         // 0x800E876C
u8*    ObjDbLookup(void* db, s32 id);                                 // 0x800D39BC
u8*    ObjRefDtor(u8* ref, s32 flags);                                // 0x800D50D0
u32    PadGetHeld(s32 chan);                                          // 0x801B9B98
s32    PartActionStat0(PartInstance* p, u32 actionKey);               // 0x8021DBE8
s32    PartActionStat1(PartInstance* p, u32 actionKey);               // 0x8021DE10
BOOL   PartAllowedAtRank(void* coll, s32 defIndex, s32 tier);         // 0x802191CC
s32    PartComputeActionCost(PartInstance* p, u32 actionKey, void* monster);  // 0x8021E6EC (really returns f32)
void   PartDestroy(PartInstance* p, s32 del);                         // 0x8021D374
void   PartInit(PartInstance* p);                                     // 0x8021D320
void   PartSetFromDef(PartInstance* p, const PartDef* d);             // 0x8021D708
void   PartSetTier(PartInstance* p, u8 tier);                         // 0x8021DA68
void   StickRead(s32 channel, f32* x, f32* y);                        // 0x80147704
void   TargetListDecode(void* t, const u32* mask, s32 side);          // 0x80216240
void*  TargetListDestroy(void* t, s32 del);                           // 0x80216200
void   TargetListInit(void* t);                                       // 0x802161DC
s32    WorldGetPhase(void* world);                                    // 0x800D83A8
s32    fn_8006C664(u8* obj);                                          // 0x8006C664
void   fn_8006C66C(u8* obj, s32 state);                               // 0x8006C66C
void   fn_8006C6AC(u8* obj);                                          // 0x8006C6AC
void   fn_8006CB54(u8* out, u8* obj);                                 // 0x8006CB54 (writes a vec3)
void   fn_800DB370(void* cam, f32* a, f32* b, f32* c);                // 0x800DB370
s32    fn_801088F0(u8* p, s32 a, s32* out);                           // 0x801088F0
u32    fn_80114E14(void);                                             // 0x80114E14
void   fn_801420EC(void);                                             // 0x801420EC
void   fn_801458B0(f32* quat, f32* a, f32* b);                        // 0x801458B0 (look-at quaternion)
void*  fn_801EEB20(u8* heap, u32 size);                               // 0x801EEB20 (heap alloc)
void   fn_801EF070(u8* heap, void* p);                                // 0x801EF070 (heap free)
void   fn_801F5714(s32 flags);                                        // 0x801F5714
void   fn_801F5814(s32 a);                                            // 0x801F5814

extern u8      gGame[];                // 0x803B53A0
extern u8*     gGlobalSettings;        // 0x805FB7A4 (pointer to the settings block)
extern void*   gLab;                   // 0x805FBBC8
extern void*   gObjDb;                 // 0x805FB180
extern s32     lbl_803C4E28[];         // 0x803C4E28 (combat part zone-loaded flags)
extern u8      lbl_803C4298[];         // 0x803C4298 (+4: camera object, +0x38 controller)
extern u8      lbl_8047CAC0[];         // 0x8047CAC0 (engine heaps)
extern f32     lbl_805F90D0;           // 0x805F90D0
extern s32     lbl_805F95F8;           // 0x805F95F8 (current memory pool index, -1 = none)
extern s32     lbl_805FB034;           // 0x805FB034
extern s32     lbl_805FB038;           // 0x805FB038 (number of combat part zones)
extern u8      lbl_805FB211;           // 0x805FB211
extern f32     lbl_805FB424;           // 0x805FB424
extern f32     lbl_805FB428;           // 0x805FB428
extern s32     lbl_805FB438;           // 0x805FB438
extern s32*    lbl_805FB7A0;           // 0x805FB7A0 (per-pool allocation counters)
extern u8      lbl_805FB7A8[];         // 0x805FB7A8 (+4: second heap enabled)
extern u8*     lbl_805FBBE0;           // 0x805FBBE0 (save block pointer)
}

// Copies the PartDef image word by word, as the original does.
static void CopyPartDef(PartDef* dst, const PartDef* src) {
    s32 i;
    for (i = 0; i < 0x50; i++) {
        ((u8*)dst)[i] = ((const u8*)src)[i];
    }
}

// Cost shown for action 1 (second == 0) or action 2 (second != 0) of part definition a0: in the
// recipe screen the computed cost of a fresh part at the current tier, otherwise the absolute
// value of the action record's cost byte.
static ScriptValue PartActionCost(s32 a0, s32 second) {
    PartDef def;
    PartDef copy;
    PartInstance inst;
    PartAction act;
    s32 actionId;
    u8 tier;
    s32 cost;

    if (a0 >= 0 && a0 < 0xBA) {
        PartDbGetDef(&def, gGameDb, a0);
        CopyPartDef(&copy, &def);
        actionId = second ? copy.action2 : copy.action1;
        act = ((PartAction*)*(u8**)(gGameDb->data + 0x430))[actionId];
        if (CurrentScreen_Recipes() != 0) {
            tier = (u8)fn_80114E14();
            PartInit(&inst);
            PartSetFromDef(&inst, &copy);
            PartSetTier(&inst, tier);
            cost = PartComputeActionCost(&inst, (u32)actionId, NULL);
            PartDestroy(&inst, -1);
            return cost;
        }
        cost = *(s8*)((u8*)&act + 0x10);
        return cost < 0 ? -cost : cost;
    }
    return 0;
}

// Value shown for action 1 / action 2 of part definition a0: in the recipe screen the stat of a
// fresh part (stat type 0 = PartActionStat0, 1 = PartActionStat1, otherwise 0), otherwise the
// absolute value of the action record's halfword at +0xE.
static ScriptValue PartActionValue(s32 a0, s32 second) {
    PartDef def;
    PartDef copy;
    PartInstance inst;
    PartAction act;
    s32 actionId;
    s32 statKey;
    u8 tier;
    s32 v;

    if (a0 >= 0 && a0 < 0xBA) {
        PartDbGetDef(&def, gGameDb, a0);
        CopyPartDef(&copy, &def);
        actionId = second ? copy.action2 : copy.action1;
        act = ((PartAction*)*(u8**)(gGameDb->data + 0x430))[actionId];
        statKey = *(s32*)(*(u8**)(gGameDb->data + 0x8) + act.type * 0x24 + 0x8);
        if (CurrentScreen_Recipes() != 0) {
            tier = (u8)fn_80114E14();
            PartInit(&inst);
            PartSetFromDef(&inst, &copy);
            PartSetTier(&inst, tier);
            if (statKey == 0) {
                v = PartActionStat0(&inst, (u32)actionId);
                PartDestroy(&inst, -1);
                return v;
            }
            if (statKey == 1) {
                v = PartActionStat1(&inst, (u32)actionId);
                PartDestroy(&inst, -1);
                return v;
            }
            PartDestroy(&inst, -1);
            return 0;
        }
        v = *(s16*)((u8*)&act + 0xE);
        return v < 0 ? -v : v;
    }
    return 0;
}

// True when limb a1 is one of the limbs hit by action 1 / action 2 of part definition a0 (the
// mask comes from the action record at +0x08, the side is the part's slot).
static s32 PartActionIsTarget(s32 a0, s32 a1, s32 second) {
    PartDef def;
    PartDef copy;
    TargetListBuf list;
    PartAction act;
    u32 mask;
    s32 i;

    if (a0 >= 0 && a0 < 0xBA) {
        PartDbGetDef(&def, gGameDb, a0);
        CopyPartDef(&copy, &def);
        act = ((PartAction*)*(u8**)(gGameDb->data + 0x430))[second ? copy.action2 : copy.action1];
        TargetListInit(&list);
        mask = act.unk08;
        TargetListDecode(&list, &mask, copy.slot);
        for (i = 0; i < list.count; i++) {
            if (a1 == list.limbs[i]) {
                TargetListDestroy(&list, -1);
                return 1;
            }
        }
        TargetListDestroy(&list, -1);
    }
    return 0;
}

extern "C" {

// 0x8001D4FC
// Refreshes the highlight state of the five part slots of the selection screen: a slot whose
// current part is not allowed at the monster's rank is shown as state 7 (6 when not selected),
// the selected slot gets state 5, others none. Then moves each slot's marker to its object.
void fn_8001D4FC(u8* mgr) {
    u8* p;
    s32 i;
    s32 want;
    s32 other;
    s32 pid;
    s32 cnt;
    s32 pos;
    u8* rec;
    u8* obj28;
    u8* obj27;
    u8* attach;
    f32 v[4];

    if (*(s32*)mgr != 0) {
        p = mgr;
        for (i = 0; i < 5; i++) {
            want = 5;
            other = -1;
            cnt = *(s32*)(p + 0x1A0);
            obj27 = *(u8**)(p + 0x1A8);
            obj28 = *(u8**)(*(u8**)mgr + i * 4 + 0xC50);
            if (cnt > 0 && (pos = *(s32*)(p + 0x198)) < cnt) {
                pid = *(s32*)(p + pos * 4 + 8);
            } else {
                pid = -1;
            }
            if (pid >= 0 && pid < *(s32*)(lbl_805FBBE0 + 0x10EC)) {
                rec = *(u8**)(lbl_805FBBE0 + pid * 4 + 0xA58);
                if (rec != NULL && PartAllowedAtRank(gLab, *(s32*)rec, *(u8*)(rec + 0x11) - 1) == 0) {
                    want = 7;
                    other = 6;
                }
            }
            if (*(s32*)(mgr + 0xA80) == i) {
                if (obj28 != NULL && want != fn_8006C664(obj28)) {
                    fn_8006C66C(obj28, want);
                }
                if (obj27 != NULL && want != fn_8006C664(obj27)) {
                    fn_8006C66C(obj27, want);
                }
            } else {
                if (obj28 != NULL && other != fn_8006C664(obj28)) {
                    if (other != -1) {
                        fn_8006C66C(obj28, other);
                    } else {
                        fn_8006C6AC(obj28);
                    }
                }
                if (obj27 != NULL && other != fn_8006C664(obj27) && fn_8006C664(obj27) != 8) {
                    if (other != -1) {
                        fn_8006C66C(obj27, other);
                    } else {
                        fn_8006C6AC(obj27);
                    }
                }
            }
            if (obj27 != NULL) {
                fn_8006CB54((u8*)v, obj27);
                v[3] = 1.0f;
                attach = *(u8**)(obj27 + 0x90);
                *(f32*)(attach + 0x38) = v[0];
                *(f32*)(attach + 0x3C) = v[1];
                *(f32*)(attach + 0x40) = v[2];
                *(f32*)(attach + 0x44) = v[3];
            }
            p += 0x1A4;
        }
        UpdateAllZoneEntities();
    }
}

// 0x8019BFF8
// Walks the child tree of a widget (children at +0x120, siblings at +0x124): cb(child, arg) returns
// 0 to descend into the child, -7 to skip it, anything else (-6) to stop this level.
void fn_8019BFF8(void* obj, void* cb, void* arg) {
    u8* c;
    s32 r;

    for (c = *(u8**)((u8*)obj + 0x120); c != NULL; c = *(u8**)(c + 0x124)) {
        r = ((TreeVisitFn)cb)(c, arg);
        if (r == 0) {
            fn_8019BFF8(c, cb, arg);
        } else if (r != -7) {
            break;
        }
    }
}

// 0x801D4790
// Heap-ownership check for pointer p: 1 when the block lies in the tracked heap (and bumps the
// per-pool counter when pool tracking is on), 0 when it lies in one of the small-block pools.
s32 fn_801D4790(u8* p) {
    s32 slot = 0;
    s32 inRange = 0;
    s32 other = 0;

    if (gGlobalSettings == NULL) {
        if (p != NULL && p >= *(u8**)(lbl_8047CAC0 + 0x37C) - 0x10 && p < *(u8**)(lbl_8047CAC0 + 0x384)) {
            inRange = 1;
        }
        if (inRange != 0) {
            return 0;
        }
        if (*(s32*)(lbl_805FB7A8 + 4) != 0) {
            if (p != NULL) {
                other = 0;
                if (p >= *(u8**)(lbl_8047CAC0 + 0x710) - 0x10 && p < *(u8**)(lbl_8047CAC0 + 0x718)) {
                    other = 1;
                }
            } else {
                other = 0;
            }
            if (other != 0) {
                return 0;
            }
        }
        return 1;
    }
    if ((u32)(ScriptValue)p >= 0x90400000u &&
        (u32)(ScriptValue)p <= (u32)(*(s32*)gGlobalSettings + 0x90400000)) {
        inRange = 1;
    }
    if (inRange != 0) {
        return 1;
    }
    if ((u32)(ScriptValue)p >= 0x90400000u &&
        (u32)(ScriptValue)p <= (u32)(*(s32*)(gGlobalSettings + 0x30) + 0x90400000)) {
        other = 1;
    }
    if (other != 0 && lbl_805FB211 == 0) {
        return 1;
    }
    if (fn_801088F0(p, -1, &slot) != 0) {
        if (lbl_805FB211 != 0) {
            if (lbl_805F95F8 >= 0 && fn_801088F0(p, lbl_805F95F8, NULL) != 0) {
                lbl_805FB7A0[slot] = lbl_805FB7A0[slot] + 1;
                return 1;
            }
            fn_801088F0(p, 0, NULL);
            lbl_805FB7A0[slot] = lbl_805FB7A0[slot] + 1;
            return 1;
        }
        return 0;
    }
    return 0;
}

// 0x801EF6E8
// Aligned allocation from a heap: alignment up to 16 uses the plain allocator; otherwise the
// alignment is rounded up to a power of two, an over-sized block is allocated, and the front
// (and tail) padding is split off and freed.
void* fn_801EF6E8(u8* heap, u32 align, u32 size) {
    u32 need;
    u8* mem;
    u8* blk;
    u8* aligned;
    u8* tail;
    u32 w;
    u32 gap;
    u32 rest;
    u32 sz;
    u32 pow;

    if (align <= 0x10) {
        return fn_801EEB20(heap, size);
    }
    if (align & (align - 1)) {
        pow = 0x20;
        while (pow < align) {
            pow *= 2;
        }
        align = pow;
    }
    if (size >= (u32)-0x21) {
        return NULL;
    }
    need = 0x10;
    if (size + 0x13 >= 0x10) {
        need = (size + 0x13) & ~0xFu;
    }
    mem = (u8*)fn_801EEB20(heap, need + align + 0x10);
    if (mem == NULL) {
        return NULL;
    }
    blk = mem - 8;
    if ((uintptr_t)mem % align != 0) {
        aligned = (u8*)(((uintptr_t)mem + align - 1) & -(intptr_t)align) - 8;
        if ((uintptr_t)(aligned - blk) < 0x10) {
            aligned += align;
        }
        w = *(u32*)(blk + 4);
        gap = (u32)(aligned - blk);
        rest = (w & ~3u) - gap;
        if (w & 2) {
            *(u32*)aligned = *(u32*)blk + gap;
            *(u32*)(aligned + 4) = rest | 2;
            return aligned + 8;
        }
        tail = aligned + rest;
        *(u32*)(aligned + 4) = rest | 1;
        *(u32*)(tail + 4) = *(u32*)(tail + 4) | 1;
        *(u32*)(blk + 4) = (*(u32*)(blk + 4) & 1) | gap;
        fn_801EF070(heap, blk + 8);
        blk = aligned;
    }
    w = *(u32*)(blk + 4);
    if (!(w & 2)) {
        sz = w & ~3u;
        if (sz > need + 0x10) {
            tail = blk + need;
            *(u32*)(tail + 4) = (sz - need) | 1;
            *(u32*)(blk + 4) = (*(u32*)(blk + 4) & 1) | need;
            fn_801EF070(heap, tail + 8);
        }
    }
    return blk + 8;
}

// 0x800F8344
// Teleports the character with id a0 to (a1, a2, a3): writes the position and a look-at
// quaternion (forward +Z, up +Y) into its sub-record at +0x4 and flags it as moved.
void MoveEntityToLocation(u32 a0, f32 a1, f32 a2, f32 a3) {
    f32 up[3];
    f32 fwd[3];
    f32 q[4];
    u8* ent;
    u8* rec;

    ent = (u8*)CharacterFind((s32)a0);
    fwd[0] = 0.0f;
    fwd[1] = 0.0f;
    fwd[2] = 1.0f;
    up[0] = 0.0f;
    up[1] = 1.0f;
    up[2] = 0.0f;
    fn_801458B0(q, up, fwd);
    rec = *(u8**)(ent + 0x4);
    *(f32*)(rec + 0x0) = a1;
    *(f32*)(rec + 0x4) = a2;
    *(f32*)(rec + 0x8) = a3;
    rec = *(u8**)(ent + 0x4);
    *(u32*)(rec + 0x20) |= 0x8000;
    rec = *(u8**)(ent + 0x4);
    *(u32*)(rec + 0x20) |= 0x10000000;
    rec = *(u8**)(ent + 0x4);
    *(u32*)(rec + 0x20) |= 0x4000;
    rec = *(u8**)(ent + 0x4);
    *(u32*)(rec + 0x20) |= 0x10000000;
    rec = *(u8**)(ent + 0x4);
    *(f32*)(rec + 0xC) = q[0];
    *(f32*)(rec + 0x10) = q[1];
    *(f32*)(rec + 0x14) = q[2];
    *(f32*)(rec + 0x18) = q[3];
    rec = *(u8**)(ent + 0x4);
    *(u32*)(rec + 0x20) |= 0x8000;
    rec = *(u8**)(ent + 0x4);
    *(u32*)(rec + 0x20) |= 0x10000000;
    rec = *(u8**)(ent + 0x4);
    *(u32*)(rec + 0x20) |= 0x4000;
    rec = *(u8**)(ent + 0x4);
    *(u32*)(rec + 0x20) |= 0x10000000;
}

// 0x801243B8
// Rotates vec3 *in by the 3x3 basis stored column-wise in the matrix (columns at +0x0, +0xC and
// +0x18). When the second column points almost straight down (-m[5] > 0.9) the second and
// third columns are swapped (third negated) to avoid a degenerate basis.
void MtxMultVec(const f32* m, const Vec3* in, Vec3* out) {
    f32 x = in->x;
    f32 y = in->y;
    f32 z = in->z;
    f32 d = (m[5] * -1.0f) + ((m[3] * -0.0f) + (m[4] * -0.0f));

    if (d > 0.9f) {
        out->x = ((-m[3]) * z) + ((m[0] * x) + (m[6] * y));
        out->y = ((-m[4]) * z) + ((m[1] * x) + (m[7] * y));
        out->z = ((-m[5]) * z) + ((m[2] * x) + (m[8] * y));
        return;
    }
    out->x = (m[6] * z) + ((m[0] * x) + (m[3] * y));
    out->y = (m[7] * z) + ((m[1] * x) + (m[4] * y));
    out->z = (m[8] * z) + ((m[2] * x) + (m[5] * y));
}

// 0x8013F034
// Resets the multiplayer state: clears the combat part zone flags, the zone counters, audio and
// friend-list state, and the pending response and cursor values.
void MultiPlayerInit(void) {
    s32 i;

    for (i = 0; i < lbl_805FB038; i++) {
        lbl_803C4E28[i] = 0;
    }
    lbl_805FB038 = 0;
    lbl_805FB034 = 0;
    fn_801F5714(-1);
    fn_801F5814(0);
    PauseSFX(1U);
    FriendListPreInit();
    lbl_805F90D0 = 10.0f;
    fn_801420EC();
    lbl_805FB438 = 0;
    lbl_805FB424 = 0.0f;
    lbl_805FB428 = 0.0f;
    MultiplayerCleanup();
}

// 0x80122BD8
// Polls one nunchuk (pad index at +0x18, valid 0..3): +0/+4/+8 = normalized stick direction,
// +0xC = stick magnitude clamped to 1, +0x10 = button bits (previous in +0x14).
void NunchukUpdate(void* nunchuk) {
    u8* n = (u8*)nunchuk;
    f32 sx;
    f32 sy;
    f32 len;
    f32 len2;
    f32 inv;
    f32 d[3];
    s32 pad = *(s32*)(n + 0x18);

    if (pad >= 0 && pad < 4) {
        *(f32*)(n + 0x0) = 0.0f;
        *(f32*)(n + 0x4) = 0.0f;
        *(f32*)(n + 0x8) = 0.0f;
        *(f32*)(n + 0xC) = 0.0f;
        *(s32*)(n + 0x14) = *(s32*)(n + 0x10);
        *(s32*)(n + 0x10) = 0;
        StickRead(*(s32*)(lbl_803C4298 + 0x38) * 0x10 + 0xE, &sx, &sy);
        *(f32*)(n + 0x0) = sx;
        if (sx != sx) {
            *(f32*)(n + 0x0) = 0.0f;
        }
        *(f32*)(n + 0x4) = sy;
        if (sy != sy) {
            *(f32*)(n + 0x4) = 0.0f;
        }
        len2 = (*(f32*)(n + 0x8) * *(f32*)(n + 0x8)) +
               ((*(f32*)(n + 0x0) * *(f32*)(n + 0x0)) + (*(f32*)(n + 0x4) * *(f32*)(n + 0x4)));
        len = 0.0f;
        if (len2 == 0.0f) {
            d[2] = 0.0f;
            d[1] = 0.0f;
            d[0] = 0.0f;
        } else {
            len = sqrtf(len2);
            inv = 1.0f / len;
            d[0] = *(f32*)(n + 0x0) * inv;
            d[1] = *(f32*)(n + 0x4) * inv;
            d[2] = *(f32*)(n + 0x8) * inv;
        }
        *(f32*)(n + 0x0) = d[0];
        *(f32*)(n + 0x4) = d[1];
        *(f32*)(n + 0x8) = d[2];
        if (len >= 1.0f) {
            len = 1.0f;
        }
        *(f32*)(n + 0xC) = len;
        if (PadGetHeld(*(s32*)(n + 0x18)) & 0x40) {
            *(s32*)(n + 0x10) |= 1;
        }
        if (PadGetHeld(*(s32*)(n + 0x18)) & 0x100) {
            *(s32*)(n + 0x10) |= 2;
        }
        if (PadGetHeld(*(s32*)(n + 0x18)) & 0x200) {
            *(s32*)(n + 0x10) |= 4;
        }
        if (PadGetHeld(*(s32*)(n + 0x18)) & 0x80) {
            *(s32*)(n + 0x10) |= 8;
        }
        if (PadGetHeld(*(s32*)(n + 0x18)) & 0x20) {
            *(s32*)(n + 0x10) |= 0x10;
        }
    }
}

// 0x800F6134
// Pans the camera to junction a0 during the exploration phase: aims it 12.5 units back along the
// camera's current view direction from the junction's path position, then blends over a1.
void PanCameraToJunction(u32 a0, f32 a1) {
    u32 rec[0x101];
    f32 camUp[3];
    f32 view[3];
    f32 camAux[3];
    f32 dir[3];
    f32 eye[3];
    f32 pos[3];
    Vec3* pp;
    u8* src;
    u8* cam;
    f32 len2;
    f32 inv;
    s32 i;

    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        src = ObjDbLookup(gObjDb, (s32)a0);
        for (i = 0; i < 0x101; i++) {
            rec[i] = *(u32*)(src + i * 4);
        }
        pp = PathGetPosition((PathFollower*)rec);
        pos[0] = pp->x;
        pos[1] = pp->y;
        pos[2] = pp->z;
        cam = *(u8**)(lbl_803C4298 + 4);
        fn_800DB370(cam, camUp, view, camAux);
        len2 = (view[2] * view[2]) + ((view[0] * view[0]) + (view[1] * view[1]));
        if (len2 == 0.0f) {
            dir[2] = 0.0f;
            dir[1] = 0.0f;
            dir[0] = 0.0f;
        } else {
            inv = 1.0f / sqrtf(len2);
            dir[0] = view[0] * inv;
            dir[1] = view[1] * inv;
            dir[2] = view[2] * inv;
        }
        view[0] = dir[0] * 12.5f;
        view[1] = dir[1] * 12.5f;
        view[2] = dir[2] * 12.5f;
        eye[0] = pos[0] - view[0];
        eye[1] = pos[1] - view[1];
        eye[2] = pos[2] - view[2];
        cam = *(u8**)(lbl_803C4298 + 4);
        (*(CamModeFn*)(*(u8**)cam + 0xC))(cam, 6);
        cam = *(u8**)(lbl_803C4298 + 4);
        (*(CamLookFn*)(*(u8**)cam + 0x2C))(cam, eye, pos);
        cam = *(u8**)(lbl_803C4298 + 4);
        (*(CamBlendFn*)(*(u8**)cam + 0x30))(cam, a1);
        ObjRefDtor((u8*)rec, -1);
    }
}

// 0x800F2974
ScriptValue PartAction1GetCost(s32 a0) {
    return PartActionCost(a0, 0);
}

// 0x800F3094
s32 PartAction1GetIsTarget(s32 a0, s32 a1) {
    return PartActionIsTarget(a0, a1, 0);
}

// 0x800F2B48
ScriptValue PartAction1GetValue(s32 a0) {
    return PartActionValue(a0, 0);
}

// 0x800F35C4
ScriptValue PartAction2GetCost(s32 a0) {
    return PartActionCost(a0, 1);
}

// 0x800F3CEC
s32 PartAction2GetIsTarget(s32 a0, s32 a1) {
    return PartActionIsTarget(a0, a1, 1);
}

// 0x800F3928
ScriptValue PartAction2GetValue(s32 a0) {
    return PartActionValue(a0, 1);
}

}  // extern "C"
