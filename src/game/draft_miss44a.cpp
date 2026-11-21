// Functions rewritten from build/drafts/draft_miss44.cpp (first 20 by file order, TestFailed skipped).
// Victory screen setup, combat-exit check, particle spawning at children/junctions/locators,
// pooled entities, NIS frame FX, quest timer, Wii input game, stitching-bug sounds, SFX stop helpers,
// laser cleanup and TimerGetSeconds. Every body is checked against the asm (sh tools/dump.sh).
#include "types.h"
#include "revolution.h"
#include "engine/math.h"
#include "game/character.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
// Callees (addresses from symbols.txt / the bl targets).
typedef s32 (*VFnS32)(void* self);
typedef void* (*VFnObjHash)(void* self, u32 hash);

void*       ScreenSetsFind(void* sets, s32 zone);                 // 0x8018D1EC
void*       ScreenSetFindWidget(void* set, u32 hash);             // 0x80198E34
void*       UIWidgetFindChild(void* widget, u32 hash);            // 0x80199AEC
ScriptValue fn_801A1208(u8* el);                                  // 0x801A1208
void        fn_801FBE38(s32 n, void* a, void* b, s32 c, void* d); // 0x801FBE38
void        fn_80042B0C(void);                                    // 0x80042B0C
void        fn_80042B30(void);                                    // 0x80042B30
void        fn_80042BE8(void);                                    // 0x80042BE8
void        fn_800429C0(void);                                    // 0x800429C0
BOOL        fn_801F56FC(s32 flags);                               // 0x801F56FC
BOOL        fn_80142358(void);                                    // 0x80142358
u32         fn_80142468(void);                                    // 0x80142468
void*       EventPost(s32 id, void* sender);                      // 0x801F3918
void        fn_801EC8C4(u8* p, u32 a, u32 b);                     // 0x801EC8C4
void        EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d); // 0x801BA190
s32         WorldGetPhase(void* world);                           // 0x800D83A8
u8*         ObjDbLookup(void* db, s32 id);                        // 0x800D39BC
u8*         ObjRefDtor(u8* ref, s32 flags);                       // 0x800D50D0
void        fn_800D5B1C(u8* obj, s32 a0, f32* pos);             // 0x800D5B1C
void        fn_801ED788(f32* out, u8* a1);                        // 0x801ED788
void*       fn_8010C90C(u32 pool, f32* pos, s32 flag);            // 0x8010C90C
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner); // 0x801C80E0
extern "C++" void* SpawnHelperAttached(void* a, void* owner, void* b);              // 0x8000C508 (game_spawn.cpp)
s32         fn_8016195C(void* p, s32 a, s32 b, void* c);          // 0x8016195C
extern "C++" void SessionSetTimer(u8* session, f32 seconds, u8 flag);   // 0x80012C2C (session.cpp)
void        ControlsLock(void* player, int a);                    // 0x8004CD28
void        ExplorationPromptClear(BOOL animate);                 // 0x80016EA0
void        fn_8020A474(s32 handle);                              // 0x8020A474
void        fn_8011D69C(void* obj, s32 a);                        // 0x8011D69C
void        fn_801FF2A8(u8* audio, s32 handle);                   // 0x801FF2A8
void        fn_801FF47C(u8* audio);                               // 0x801FF47C
s32         fn_801C3E88(u8* tbl, void* handle);                                   // 0x801C3E88
void*       fn_801D40E0(s32 n);                                   // 0x801D40E0
void*       fn_80209814(void* v);                                 // 0x80209814
s32         PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e); // 0x80209E70
f64         fn_8032F9E8(u32 hi, u32 lo);                          // 0x8032F9E8 (u64 to f64)

// Globals (addresses in the names or comments).
extern u8   gGame[];
extern u8   gWorldState[];   // 0x803E6D00 (first word is a pointer to the world block)
extern u8   gAudio[];
extern void* gObjDb;
extern "C++" void* gEventMgr;
extern "C++" void* gScreenSets;
extern u8   lbl_805FB068;    // 0x805FB068
extern u8   lbl_805FB069;    // 0x805FB069
extern u8   lbl_805FB06A;    // 0x805FB06A
extern s32  lbl_805FB178;    // 0x805FB178
extern s32  lbl_805FB17C;    // 0x805FB17C
extern s32  lbl_805F8C38;    // 0x805F8C38
extern s32  lbl_805F8C3C;    // 0x805F8C3C
extern void* lbl_805FBBAC;   // 0x805FBBAC
extern s32  lbl_805FB6AC;    // 0x805FB6AC
extern s32  lbl_805FB6B8;    // 0x805FB6B8
extern u8   lbl_805FB248[];  // 0x805FB248 (lab books section table pointer)
extern s32  lbl_805FB1D8[];  // 0x805FB1D8 (2 entries)
extern s32  lbl_805FB1E0[];  // 0x805FB1E0 (2 entries)
extern s32  lbl_8047C7AC;    // 0x8047C7AC
extern u8   lbl_8047C760[];  // 0x8047C760 (particle effect manager)
extern u8   lbl_8040E5F0[];  // 0x8040E5F0 (NIS effect manager)
extern u8   lbl_80559D78[];  // 0x80559D78 (SFX voice table, 256 x 0x28)
}

extern "C" {

// 0x800428C0
void SetupVictoryScreenShot(s32 a0) {
    (void)a0;
    if (lbl_805FB068 != 0) return;
    for (s32 zone = 0; zone < 2; zone++) {
        void* set = ScreenSetsFind(gScreenSets, zone);
        if (set == NULL) continue;
        void* widget = ScreenSetFindWidget(set, 0xB93CA1BDu);
        if (widget == NULL) continue;
        void* child = UIWidgetFindChild(widget, 0x1B49766Fu);
        if (child == NULL) continue;
        if (fn_801A1208((u8*)child) == 0) return;
        fn_801A1208((u8*)child);
        fn_801FBE38(0x14, (void*)fn_80042B0C, (void*)fn_80042B30, 0, (void*)fn_80042BE8);
        GameFlowSignal(0xDED2B2, 0, 0);
        lbl_805FB068 = 1;
        lbl_805FB06A = 0;
        lbl_805FB069 = 0;
        break;
    }
    fn_800429C0();
}


// Shared body of SpawnAttachedParticleAtChild / SpawnOneParticleAtChild. desc is a spawn descriptor:
// u16 flags at +0 (bit 1 = a child slot index follows), u8 slot index at +2, u32 at +0 copied into
// the spawned event. The effect is posted to the owner and the owner's slot at +0x94 is updated.
static void SpawnParticleOnChildSlot(void* owner, u8* desc, u32 a2) {
    if (owner == NULL) return;
    s32 idx = -1;
    if (*(u16*)desc & 0x2) idx = *(u8*)(desc + 2);
    if (idx < 0) return;
    u8* ev = (u8*)EventPost(0x1A, owner);
    if (ev != NULL) {
        *(u32*)(ev + 0x4) = a2;
        *(s32*)(ev + 0x8) = 1;
        *(u32*)(ev + 0xC) = *(u32*)desc;
    }
    fn_801EC8C4((u8*)owner + 0x94, (u32)idx, a2);
}

// 0x800E9AE4
void SpawnAttachedParticleAtChild(void* a0, u32 a1, u32 a2) {
    SpawnParticleOnChildSlot(a0, (u8*)(uintptr_t)a1, a2);
}

// 0x800F781C
void SpawnCoffinProjectile(void) {
    u8* game = *(u8**)(gGame + 0xD614);
    u8* pos = *(u8**)(game + 0x4);
    f32 x = *(f32*)(pos + 0x0);
    f32 y = *(f32*)(pos + 0x4);
    f32 z = *(f32*)(pos + 0x8);
    f32 top = 25.0f + z;
    f32 a[3] = { x, y, top };
    f32 b[3] = { x - x, y - y, z - top };
    SpawnHelperAttached(a, (void*)(uintptr_t)*(s32*)(*(u8**)gWorldState + 0x74), b);
}

// 0x800E9A3C
void SpawnOneParticleAtChild(void* a0, u32 a1, u32 a2, s32 a3) {
    (void)a3;
    SpawnParticleOnChildSlot(a0, (u8*)(uintptr_t)a1, a2);
}

// 0x800E9744
void SpawnParticleAtJunction(u32 a0, u32 a1) {
    if (WorldGetPhase(gGame + 0xD5F0) != 2) return;
    u8* obj = ObjDbLookup(gObjDb, (s32)a1);
    // Copy the 0x408-byte object block (word by word) onto the stack.
    u32 words[0x102];
    for (s32 off = 0; off <= 0x404; off += 4) *(u32*)((u8*)words + off) = *(u32*)(obj + off);
    Vec3* p = PathGetPosition((PathFollower*)words);
    f32 pos[3] = { p->x, p->y, p->z };
    fn_800D5B1C((u8*)words, (s32)a0, pos);
    ObjRefDtor((u8*)words, -1);
}

// 0x800FB4EC
void SpawnParticleAtLocator(u32 a0, u32 a1) {
    u8* world = *(u8**)gWorldState;
    s32 count = *(s32*)(world + 0x24);
    u8* list = *(u8**)(world + 0x28);
    s32 idx = -1;
    for (s32 i = 0; i < count; i++) {
        if (*(u32*)(list + i * 0x20) == a1) {
            idx = i;
            break;
        }
    }
    if (idx >= 0 && idx < count) {
        u8* e = list + idx * 0x20;
        f32 pos[4];
        pos[0] = *(f32*)(e + 0x8);
        pos[1] = *(f32*)(e + 0xC);
        pos[2] = *(f32*)(e + 0x10);
        pos[3] = 0.0f;
        SpawnEffectAt(lbl_8047C760, a0, pos, 0);
    }
}

// 0x8010CBFC
void SpawnPooledEntityAt(u32 a0, u32 a1) {
    f32 pos[3];
    fn_801ED788(pos, (u8*)(uintptr_t)a1);
    u8* p = (u8*)fn_8010C90C(a0, pos, 0);
    if (p == NULL) return;
    *(s32*)(p + 0x30) = lbl_805FB6AC;
    lbl_805FB6AC = lbl_805FB6AC + 1;
    u8* ev = (u8*)EventPost(4, NULL);
    if (ev != NULL) {
        *(s32*)(ev + 0xC) = *(s32*)(p + 0x30);
        *(u32*)(ev + 0x8) = a0;
        *(u32*)(ev + 0x4) = *(u32*)(uintptr_t)a1;
    }
}



// 0x800F49E8
void StartQuestTimer(s32 a0, u32 a1) {
    // The original converts a0 to a float with the 2^52 magic-number trick; that is exact for s32.
    SessionSetTimer(gGame + 0x5C8, (f32)a0, (a1 != 0) ? 1 : 0);
}

// 0x800F6D48
void StartWiiInputGame(void) {
    ControlsLock(*(u8**)(gGame + 0xD614), 1);
    for (s32 i = 0; i < 0x20; i++) {
        u8* obj = *(u8**)(gGame + 0x140 + i * 4);
        if (obj == NULL) continue;
        if ((*(VFnS32*)(*(u8**)obj + 0x4C))(obj) != 0) continue;
        u8* def = *(u8**)(obj + 0x4);
        if ((*(u32*)(def + 0x1C) & 0x10) == 0) continue;
        if (*(u8*)(obj + 0x895) != 0) continue;
        EventSend(gEventMgr, obj, NULL, 0x4F9213C1u, 0, 0, 0, 0);
    }
    ExplorationPromptClear(0);
    UIScreen_Push(0xAA7F5A69u);
}


// 0x800C6B3C
void Stitching_Bug_StartEatingSound(void* a0, u32 a1) {
    (void)a0;
    s32 n = lbl_805FB17C + 1;
    lbl_805FB17C = n;
    if (n <= 0) return;
    if (lbl_805F8C3C != -1) return;
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    lbl_805F8C3C = PlaySound(a1, 0, 0, 0, 0, 0);
}

// 0x800C6A70
void Stitching_Bug_StartSkitterSound(void* a0, u32 a1) {
    (void)a0;
    s32 n = lbl_805FB178 + 1;
    lbl_805FB178 = n;
    if (n <= 0) return;
    if (lbl_805F8C38 != -1) return;
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    lbl_805F8C38 = PlaySound(a1, 0, 0, 0, 0, 0);
}


// 0x8010FC98
void StopBackgroundSFX(u32 a0, u32 a1) {
    if (a0 != 0) fn_8020A474(*(s32*)(uintptr_t)a0);
    if (a1 != 0) fn_8020A474(*(s32*)(uintptr_t)a1);
}

// 0x800EC58C
void StopLaser(void* a0, s32 a1) {
    u8* laser = (u8*)a0;
    u32 flags0 = *(u32*)(*(u8**)(laser + 4) + 0x20);
    if ((flags0 & 0x04000000u) == 0) {
        u8* ev = (u8*)EventPost(0x28, a0);
        if (ev != NULL) *(s32*)(ev + 0x4) = a1;
    }
    u32 flags = *(u32*)(*(u8**)(laser + 4) + 0x1C);
    if (flags & 0x40u) {
        if (a1 < 0 || a1 >= 2) return;
        s32 h = lbl_805FB1E0[a1];
        lbl_805FB1D8[a1] = 0;
        if (h != 0) {
            fn_801C3E88((u8*)&lbl_8047C7AC, (void*)(intptr_t)h);
            lbl_805FB1E0[a1] = 0;
        }
    } else if (flags & 0x20u) {
        if (a1 < 0 || a1 >= 2) return;
        *(s32*)(laser + a1 * 4 + 0xC2C) = 0;
        if (*(s32*)(laser + a1 * 4 + 0xC34) != 0) {
            fn_801C3E88((u8*)&lbl_8047C7AC, (void*)(intptr_t)*(s32*)(laser + a1 * 4 + 0xC34));
            *(s32*)(laser + a1 * 4 + 0xC34) = 0;
        }
    }
}

// 0x80142538
f32 TimerGetSeconds(u8* arg0) {
    u64 now = (u64)OSGetTime();
    u64 saved = ((u64)*(u32*)arg0 << 32) | *(u32*)(arg0 + 4);
    if (saved - now < 60000000ULL) {
        saved = now;
        *(u32*)arg0 = (u32)(now >> 32);
        *(u32*)(arg0 + 4) = (u32)now;
    }
    u64 elapsed = now - saved;
    f64 ticksPerSec = (f64)(OSGetBusClock() >> 2);
    return (f32)(fn_8032F9E8((u32)(elapsed >> 32), (u32)elapsed) / ticksPerSec);
}

}
