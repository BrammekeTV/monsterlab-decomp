// Small functions batch miss702 (0x8004-0x800Dxxxx). Verified against the asm.
#include "types.h"
#include "engine/math.h"
#include <math.h>

extern "C" {
#include "game/script_callbacks.h"
}

struct PathFollower;
struct ScopedLock { u8 pad[4]; };  // as in props.cpp

// Vtable / function-pointer call shapes used below.
typedef u8* (*GetRecFn)(u8* self);
typedef void (*VoidSelfFn)(u8* self);
typedef void (*VoidSelfIntFn)(u8* self, s32 a);
typedef void (*ObjVisitFn)(u8* self, u8* other);

extern "C++" void* gEventMgr;
extern "C++" void* gScreenSets;
extern void* gObjDb;
extern u8 gGame[];
extern void* lbl_805FB5F8;
extern void* lbl_805FBBAC;
extern u8 lbl_8047C760[];
extern u8 lbl_80412B20[];

extern "C" {

void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
void EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);
u8* ObjDbLookup(void* db, s32 id);
BOOL PathIsAtJunction(PathFollower* p);
s32 PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);
u32 RandomU32(void);
s32 RaySceneTest(void* ray);
void RendClear(int flags, int a, int b, f32 depth);
void ScopedLockCtor(ScopedLock* l);
void ScopedLockDtor(ScopedLock* l, s32 flags);
void* ScreenSetFindElement(void* set, u32 hash);
void* ScreenSetsCurrent(void* sets);
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);
int SysSprintf(char* buf, const char* fmt, ...);
s32 TriggerDoAction(void* action, void* a1, void* a2);
void UISetText(void* el, const char* text, s32 a);
void Vec3Copy(Vec3* out, const Vec3* v);
void Vec3Sub(Vec3* out, const Vec3* a, const Vec3* b);
u8* fn_800526DC(u8* a3);
u32 fn_80053ACC(u8* c, s32 mask);
void fn_8005CB40(u8* p, f32 t, s32 flag);
u8* fn_800633D4(u8* attachment);
f32 fn_800645AC(u8* a3, u8* a4);
void fn_800655D8(u8* arg0, f32 dt);
void fn_80065974(u8* p);
void fn_80065EC8(u8* p, f32 t);
s32 fn_80065F40(u8* p);
void fn_80065FF0(u8* arg0, f32 farg0);
u32 fn_80065FE4(u8* a3);
s32 fn_80065CB0(const f32* v);
void fn_8006680C(u8* list, const f32* vec);
void fn_8006697C(u8* p);
void fn_8006AADC(u8* p, f32 delta);
void fn_8007D860(u8* self);
void fn_8007E354(u8* arg0, u32 arg1);
void fn_80081E84(u8* self);
void fn_800B9D94(u8* p);
void fn_800BA2E8(u8* p);
void fn_800BFAC0(u8* arg0, f32* arg1);
s32 fn_800C0AE8(u8* p);
void fn_80171228(u8* a, u8* b, u8* c, f32 scale);
void fn_80182ABC(u8* p);
void fn_80183CFC(u8* p);
void fn_80183E88(u8* p);
s32 fn_801B1DD4(u8* listA, u32 idA, u8* listB, u32 idB, u8* outA, u8* outB, f32 limit);
u8* fn_801B3308(void);
void fn_801B3310(u8* p, u8* q);
void fn_801B3378(u8* p, u8* key);
s32 fn_801B52AC(u8* p);
s32 fn_801C3624(u8* arg0, u8* arg1, u8* arg2, f32* arg3, u8* arg4);
void fn_801C849C(u8* mgr, u8* item);
void* fn_801D40E0(s32 n);
s32 fn_801DA780(u8* a3);
u8* fn_801DA788(u8* a3, s32 i);
void* fn_80209814(void* v);
void fn_8020B1D4(void* handle, s32 a);

// Handle of the voice SFX; created on first use (shared by fn_8007E968).
static void EnsureVoiceHandle(void) {
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h != NULL) h = fn_80209814(h);
        lbl_805FBBAC = h;
    }
}

extern f32 lbl_805FD240;
extern f32 lbl_805FD244;
extern f32 lbl_805FD248;
extern f32 lbl_805FD468;
extern f32 lbl_805FD570;
extern f32 lbl_805FD588;
extern f32 lbl_805FD770;
extern f32 lbl_805FE03C;
extern f32 lbl_805FE234;
extern s32 lbl_80346EA0[];
extern s32 lbl_80346EB0[];
extern f32 lbl_80346EC0[];
extern f32 lbl_80346ED0[];
extern f32 lbl_80346EE0[];
extern f32 lbl_80346F00[];
extern const char lbl_805F88D8[];

// 0x8004BB64
// Scene pass: clears the screen, then for each of five slots (arg0+0xC50 + 4k) pushes the
// slot's attached records through fn_80171228, and finally resets the static record.
void fn_8004BB64(u8* arg0) {
    s32 vecA[3] = {0, 0, 0};
    f32 vecB[3] = {lbl_805FD240, lbl_805FD240, lbl_805FD248};

    RendClear(4, 0, 0, lbl_805FD244);
    fn_80182ABC(lbl_80412B20);
    for (s32 k = 4; k >= 0; k--) {
        u8* r28 = *(u8**)(arg0 + 4 * k + 0xC50);
        if (r28 == NULL) continue;

        u8* rec = (*(GetRecFn*)(*(u8**)r28 + 0xC))(r28);
        s32 count = fn_801DA780(rec);
        rec = (*(GetRecFn*)(*(u8**)r28 + 0xC))(r28);
        *(s32*)(rec + 8) |= 2;

        for (s32 i = 0; i < count; i++) {
            u8* cam = (u8*)lbl_805FB5F8;
            u8* e = fn_801DA788((*(GetRecFn*)(*(u8**)r28 + 0xC))(r28), i);
            f32 f9 = *(f32*)(e + 0x14);
            f32 f8 = *(f32*)(e + 0x18);
            f32 f7 = *(f32*)(e + 0x1C);
            f32 f6 = *(f32*)(e + 0x20);
            f32 f5 = *(f32*)(cam + 0xD0);
            f32 f4 = *(f32*)(cam + 0xD4);
            f32 f3 = *(f32*)(cam + 0xD8);
            f32 dot = (f7 * f3) + ((f9 * f5) + (f8 * f4));
            f32 scale = (*(f32*)(cam + 0xDC) - dot) + f6;
            u8* e2 = fn_801DA788((*(GetRecFn*)(*(u8**)r28 + 0xC))(r28), i);
            fn_80171228(e2, (u8*)vecA, (u8*)vecB, scale);
        }

        rec = (*(GetRecFn*)(*(u8**)r28 + 0xC))(r28);
        *(s32*)(rec + 8) &= ~2;
    }
    fn_80183CFC(lbl_80412B20);
    fn_80183E88(lbl_80412B20);
}

// 0x80065D1C
// Per-frame update of a moving object: steps its timer and position, and sends the
// junction/move helpers depending on the flags at +0x244 and the timer at +0x230.
void fn_80065D1C(u8* arg0, f32 dt) {
    if (fn_80053ACC(arg0, 1) == 0) return;

    if (*(f32*)(arg0 + 0x230) <= lbl_805FD468 && !(*(u32*)(arg0 + 0x244) & 0x20000)) {
        fn_80065EC8(arg0, dt);
        return;
    }

    u8* o228 = *(u8**)(arg0 + 0x228);
    if (*(s32*)(o228 + 0xB4) == 0) fn_80065F40(arg0);

    if (fn_80065FE4(arg0) == 0) {
        *(f32*)(arg0 + 0x230) = *(f32*)(arg0 + 0x230) - dt;
    }
    fn_80065FF0(arg0, dt);
    fn_80065974(arg0);

    if (*(void**)(arg0 + 0x25C) != NULL) {
        u8* a = fn_800526DC(arg0);
        fn_800645AC(*(u8**)(arg0 + 0x25C), a);
        fn_800633D4(*(u8**)(arg0 + 0x25C));
    }

    if (*(s32*)(arg0 + 0x234) != 0 || *(s32*)(arg0 + 0x238) != 0) {
        fn_8006697C(arg0);
    }

    if (*(void**)(arg0 + 0xA8) != NULL && !(*(u32*)(arg0 + 0x244) & 0x1000)) {
        Vec3 tmp;
        Vec3Sub(&tmp, *(Vec3**)(arg0 + 4), (Vec3*)(arg0 + 0x1A0));
        if (fn_80065CB0((const f32*)&tmp) == 0) {
            fn_8006680C(*(u8**)(arg0 + 0xA8), (const f32*)&tmp);
        }
    }

    Vec3Copy((Vec3*)(arg0 + 0x1A0), *(Vec3**)(arg0 + 4));
    Vec3Copy((Vec3*)(arg0 + 0x18), *(Vec3**)(arg0 + 4));
    fn_800655D8(arg0, dt);
    fn_8005CB40(arg0, dt, 0);
}

// 0x8006BDF4
// Pairwise interaction between two objects: when fn_801B52AC(arg1) == 6 and the distance test
// passes, fires the trigger actions attached to both; otherwise forwards to arg1's handler.
void fn_8006BDF4(u8* arg0, u8* arg1) {
    if (fn_801B52AC(arg1) != 6) {
        (*(ObjVisitFn*)(*(u8**)arg1 + 0x28))(arg1, arg0);
        return;
    }

    // Two 0x6C-byte result records (zeroed); type field 7 at +0x54.
    u8 recA[0x6C] = {};
    u8 recB[0x6C] = {};
    *(s32*)(recA + 0x54) = 7;
    *(s32*)(recB + 0x54) = 7;

    if (fn_801B1DD4(*(u8**)(arg0 + 0x8C), 0xF62113D5u, *(u8**)(arg1 + 0x8C), 0xF62113D5u,
                    recA, recB, *(f32*)(arg0 + 0x28)) == 0) {
        return;
    }

    u8* a = *(u8**)(arg0 + 0x9C);
    if (a != NULL && ((*(u32*)(a + 0x7C) >> 27) & 0xF) != 0) {
        TriggerDoAction(a + 0x7C, arg0, arg1);
    }
    if (((*(u32*)(arg0 + 0x4C)) >> 27) & 0xF) {
        TriggerDoAction(arg0 + 0x4C, arg0, arg1);
    }
    a = *(u8**)(arg1 + 0x9C);
    if (a != NULL && ((*(u32*)(a + 0x7C) >> 27) & 0xF) != 0) {
        TriggerDoAction(a + 0x7C, arg1, arg0);
    }
    if (((*(u32*)(arg1 + 0x4C)) >> 27) & 0xF) {
        TriggerDoAction(arg1 + 0x4C, arg1, arg0);
    }
}

// 0x8006D044
// Follower update: compares the object's position with the target at gGame+0xD614 and,
// when close enough and on a junction, posts an event (the flag at +0x264 selects which).
void fn_8006D044(u8* arg0, f32 dt) {
    fn_8006AADC(arg0, dt);
    s32 id = *(s32*)(arg0 + 0x268);
    if (id == 0) return;

    f32* pos = *(f32**)(arg0 + 4);
    f32* tgt = *(f32**)(*(u8**)(gGame + 0xD614) + 4);
    f32 dx = pos[0] - tgt[0];
    f32 dy = pos[1] - tgt[1];
    f32 dz = pos[2] - tgt[2];
    f32 d2 = (dz * dz) + ((dx * dx) + (dy * dy));
    // Length: sqrt of d2 when d2 exceeds the constant, otherwise the constant itself.
    f32 len = lbl_805FD570;
    if (d2 > lbl_805FD570) len = sqrtf(d2);

    if (*(u8*)(arg0 + 0x264) == 0) {
        if (len < lbl_805FD588 && PathIsAtJunction((PathFollower*)ObjDbLookup(gObjDb, id))) {
            *(u8*)(arg0 + 0x264) = 1;
            EventSend(gEventMgr, arg0, NULL, 0x59CFBC0Fu, 0, 0, 0, 0);
        }
        return;
    }

    if (!(len >= lbl_805FD588) &&
        PathIsAtJunction((PathFollower*)ObjDbLookup(gObjDb, id))) {
        return;
    }
    *(u8*)(arg0 + 0x264) = 0;
    EventSend(gEventMgr, arg0, NULL, 0x0957DD45u, 0, 0, 0, 0);
}

// 0x800750C0
// Screen update: refreshes the counter text element and the two screen-set elements whose
// hashes live in the per-slot tables at arg0+0x30 and arg0+8.
void fn_800750C0(u8* arg0) {
    void* cur = ScreenSetsCurrent(gScreenSets);
    if (cur != NULL) {
        s32 idx = *(s32*)(arg0 + 0x5C);
        u32 hash = *(u32*)(arg0 + 0x30 + idx * 4);
        if (hash != 0) {
            u8* el = (u8*)ScreenSetFindElement(cur, hash);
            if (el != NULL) {
                u8* sub = *(u8**)(el + 0x138);
                (*(VoidSelfIntFn*)(sub + 0x30))(el, 2);
            }
        }
    }

    s32 idx = *(s32*)(arg0 + 0x5C) - 1;
    if (idx < 0) idx = 0;
    *(s32*)(arg0 + 0x5C) = idx;

    cur = ScreenSetsCurrent(gScreenSets);
    if (cur != NULL) {
        u8* el = (u8*)ScreenSetFindElement(cur, 0x0281E9F9u);
        if (el != NULL) {
            s32 k = *(s32*)(arg0 + 0x5C);
            u32 v = *(u32*)(arg0 + 8 + k * 4);
            if (v != 0x10B2u) UIText_SetString_TextID(*(u32*)el, v);
        }
    }

    cur = ScreenSetsCurrent(gScreenSets);
    if (cur != NULL) {
        u8* el = (u8*)ScreenSetFindElement(cur, 0x121100C6u);
        if (el != NULL) {
            *(s32*)(arg0 + 0x60) = *(s32*)(arg0 + 0x5C);
            for (;;) {
                s32 k = *(s32*)(arg0 + 0x60);
                if (k >= 10) break;
                if (*(u32*)(arg0 + 8 + k * 4) == 0x10B2u) break;
                *(s32*)(arg0 + 0x60) = k + 1;
            }
            char buf[32];
            SysSprintf(buf, lbl_805F88D8, *(s32*)(arg0 + 0x5C) + 1);
            UISetText(el, buf, 1);
        }
    }

    cur = ScreenSetsCurrent(gScreenSets);
    if (cur != NULL) {
        s32 k = *(s32*)(arg0 + 0x5C);
        u32 h = *(u32*)(arg0 + 0x30 + k * 4);
        if (h != 0) {
            u8* el = (u8*)ScreenSetFindElement(cur, h);
            if (el != NULL) {
                u8* sub = *(u8**)(el + 0x138);
                (*(VoidSelfIntFn*)(sub + 0x2C))(el, 2);
            }
        }
    }
}

// 0x8007E968
// Tears down the per-frame effect/sound state: clears the two sub-objects, spawns or
// releases the effects for the six slots by the bit mask at +0x384, then resets the timers.
void fn_8007E968(u8* arg0) {
    fn_8007E354(arg0, 0x6CCBDF09u);

    u8* o = *(u8**)(arg0 + 0x2A8);
    if (o != NULL) (*(VoidSelfIntFn*)(*(u8**)o + 8))(o, 1);
    *(u8**)(arg0 + 0x2A8) = NULL;

    u8* o29C = *(u8**)(arg0 + 0x29C);
    (*(VoidSelfFn*)(*(u8**)o29C + 0xC0))(o29C);
    u8* o2A0 = *(u8**)(arg0 + 0x2A0);
    (*(VoidSelfFn*)(*(u8**)o2A0 + 0xC0))(o2A0);

    s32 count = *(s32*)(arg0 + 0x298);
    for (s32 k = 0; k < count; k++) {
        fn_80081E84(*(u8**)(arg0 + 4 * k + 0x1F8));
    }

    f32 f31 = lbl_805FD770;
    *(u8*)(arg0 + 0x384) = 0;
    *(f32*)(arg0 + 0x324) = f31;

    for (s32 i = 0; i < 6; i++) {
        u8 mask = *(u8*)(arg0 + 0x384);
        if ((mask >> i) & 1) {
            u8* rec = *(u8**)(arg0 + 4 * i + 0x354);
            f32 pos[4] = {*(f32*)(rec + 0x8), *(f32*)(rec + 0xC), *(f32*)(rec + 0x10), f31};
            SpawnEffectAt(lbl_8047C760, 0xFDB35BFEu, pos, (u8**)(arg0 + 0x36C + 4 * i));
            EnsureVoiceHandle();
            PlaySound(0x6158D3FDu, 0, 0, 0, 0, 1);
        } else {
            u8* item = *(u8**)(arg0 + 0x36C + 4 * i);
            if (item != NULL) {
                fn_801C849C(lbl_8047C760, item);
                *(u8**)(arg0 + 0x36C + 4 * i) = NULL;
            }
        }
    }

    fn_801B3378(fn_801B3308(), arg0 + 0xC8);
    EnsureVoiceHandle();
    fn_8020B1D4(lbl_805FBBAC, 0);
    fn_8007D860(arg0);
}

// 0x80095160
// Sends the "done" event to the object at +0x90, then to every object in the list at +0x9C
// (count at +0x1B4) and to the one at +0x94. Each target is updated under a scoped lock.
void fn_80095160(u8* arg0) {
    u8* a = *(u8**)(arg0 + 0x90);
    if (a != NULL) {
        ScopedLock lock;
        ScopedLockCtor(&lock);
        (*(VoidSelfIntFn*)(*(u8**)a + 0x14))(a, 1);
        ScopedLockDtor(&lock, -1);
        EventSend(gEventMgr, *(u8**)(arg0 + 0x90), NULL, 0x437B3D65u, 0, 0, 1, 0);
    }

    s32 count = *(s32*)(arg0 + 0x1B4);
    for (s32 i = 0; i < count; i++) {
        u8* o = *(u8**)(arg0 + 4 * i + 0x9C);
        if (o != NULL) {
            ScopedLock lock;
            ScopedLockCtor(&lock);
            (*(VoidSelfIntFn*)(*(u8**)o + 0x14))(o, 1);
            ScopedLockDtor(&lock, -1);
            EventSend(gEventMgr, o, NULL, 0x437B3D65u, 0, 0, 1, 0);
        }
    }

    u8* b = *(u8**)(arg0 + 0x94);
    if (b != NULL) {
        ScopedLock lock;
        ScopedLockCtor(&lock);
        (*(VoidSelfIntFn*)(*(u8**)b + 0x14))(b, 1);
        ScopedLockDtor(&lock, -1);
        EventSend(gEventMgr, *(u8**)(arg0 + 0x94), NULL, 0x437B3D65u, 0, 0, 1, 0);
    }
}

// 0x800AF3E0
// Returns the id (word at +0x78+4i) of the child whose position is nearest to arg1+arg2,
// skipping entries whose id is -1. Ties at the initial distance pick the first valid entry.
s32 fn_800AF3E0(u8* arg0, u8* arg1, u8* arg2) {
    f32 init = lbl_805FE03C;
    f32 best = init;
    s32 bestId = -1;
    f32* a1 = (f32*)arg1;
    f32* a2 = (f32*)arg2;
    s32 count = *(s32*)(arg0 + 0xB4);

    for (s32 i = 0; i < count; i++) {
        s32 id = *(s32*)(arg0 + 0x78 + 4 * i);
        if (id == -1) continue;
        u8* v = *(u8**)(arg0 + 4 * i);

        f32 buf[4] = {init, a1[0] + a2[0], a1[1] + a2[1], a1[2] + a2[2]};
        f32 out[3] = {init, init, init};
        fn_801C3624(v + 0x14, arg1, (u8*)&buf[1], &buf[0], (u8*)out);

        f32 dx = out[0] - *(f32*)(v + 0x14);
        f32 dy = out[1] - *(f32*)(v + 0x18);
        f32 dz = out[2] - *(f32*)(v + 0x1C);
        f32 d2 = (dz * dz) + ((dx * dx) + (dy * dy));
        if (d2 < best || best == init) {
            bestId = id;
            best = d2;
        }
    }
    return bestId;
}

// 0x800BA51C
// Reset/roll of the target: notifies two events, copies a position into the object,
// raycasts, and picks one of the table entries (with a random offset inside its range).
void fn_800BA51C(u8* arg0) {
    fn_800BA2E8(arg0);
    EventSendTo(gEventMgr, 0x83B4950Au, NULL, 0x9B51AA64u, 0, 0);
    EventSendTo(gEventMgr, 0x2A75F102u, NULL, 0x9B51AA64u, 0, 0);
    fn_800B9D94(arg0);

    f32* src = *(f32**)(*(u8**)(arg0 + 0x238) + 4);
    f32 pos[3] = {src[0], src[1], src[2]};
    fn_800BFAC0(arg0 + 0x240, pos);
    s32 rayHandle = fn_800C0AE8(arg0 + 0x240);

    *(f32*)(arg0 + 0x90) = lbl_805FE234;
    // The ray argument is the value left in r3 by fn_800C0AE8.
    s32 hit = RaySceneTest((void*)(intptr_t)rayHandle);

    *(f32*)(arg0 + 0x3AC) = lbl_80346EC0[hit];
    *(f32*)(arg0 + 0x3B0) = lbl_80346ED0[hit];
    *(f32*)(arg0 + 0x3B4) = lbl_80346EE0[hit];
    *(f32*)(arg0 + 0x3B8) = lbl_80346F00[hit];
    *(s32*)(arg0 + 0x128) = 0;
    *(s32*)(arg0 + 0x124) = 0;
    s32 cur = lbl_80346EA0[hit];
    *(s32*)(arg0 + 0x12C) = cur;

    u32 rnd = RandomU32();
    u32 span = (u32)(lbl_80346EB0[hit] - cur + 1);
    *(s32*)(arg0 + 0x12C) = cur + (s32)(rnd % span);

    fn_801B3310(fn_801B3308(), arg0 + 0x3BC);
}

}  // extern "C"
