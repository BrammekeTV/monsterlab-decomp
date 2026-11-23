// Digging puzzle, experiment and misc helpers from the m2c draft batch miss58 (verified against the asm).
#include "types.h"
#include "engine/math.h"

extern "C" {
#include "game/script_callbacks.h"

u32   StringHash(const char* s);                                    // 0x801EB704
void* ScreenSetsCurrent(void* sets);                                // 0x8018D1E4
void* ScreenSetFindElement(void* set, u32 hash);                    // 0x80198F18
void  UIWidgetPlayAnimLoop(void* w, u32 anim, s32 n);               // 0x8019BB1C
void  fn_80025524(void* obj, f32* a, f32* b);                       // 0x80025524
void  fn_8019CAD4(void* widget, u8* obj);                           // 0x8019CAD4
void  fn_800928F4(u8* p);                                           // 0x800928F4
u8*   fn_8006FBDC(void);                                            // 0x8006FBDC
s32   fn_8006FBFC(u8* p);                                           // 0x8006FBFC
u8*   fn_80093CF0(u8* p);                                           // 0x80093CF0
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner); // 0x801C80E0 (4th arg is really the owner slot pointer)
void* fn_801D40E0(s32 n);                                           // 0x801D40E0
void* fn_80209814(void* v);                                         // 0x80209814
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);      // 0x80209E70
void  fn_800D8CB0(u8* a, f32* o0, f32* o1, f32* o2, f32* o3);       // 0x800D8CB0
f32   fn_8007C5B4(u8* a3, u8* a4, u8* a5);                          // 0x8007C5B4 (leaf)
f32   fn_8007A3F4(u8* a3, u8* a4, u8* a5);                          // 0x8007A3F4 (leaf)
s32   fn_8007AC80(u8* a, f32* b, f32* c, f32* d, f32* e, f32* f, f32* g); // 0x8007AC80
void  fn_8007A8D0(u8* a, u8* out, f32* b, f32* c, f32* d, f32* e);  // 0x8007A8D0
void  fn_8007A68C(u8* a, u8* st, u8* b, u8* c);                     // 0x8007A68C
void  fn_801B4EA4(u8* obj, s32 flag);                               // 0x801B4EA4
void* fn_801DA758(void* v);                                         // 0x801DA758
void  QuatFromMtx(Quat* out, const f32 m[12]);                      // 0x801456E4
s32   RegisterScriptFunc(const char* name, s32 type, void* fn);      // 0x801DC91C
void  fn_800A7D88(void* obj, u32 hash);                             // 0x800A7D88 (send event hash to obj)
void  RobotEvolutionExperiment_RobotReadyToLaunch(u8* obj);
void  RobotEvolutionExperiment_RobotLanded(u8* obj);
void  RobotEvolutionExperiment_RobotReadyToPlay(u8* obj);
void  RobotEvolutionExperiment_RobotShowAnswer(u8* obj);
void  RobotEvolutionExperiment_RobotStartFleeCycle(u8* obj);
s32   RobotEvolutionExperiment_RobotGameOver(u8* obj);
u8*   fn_800C73EC(u8* h);                                          // 0x800C73EC
u8*   fn_800C7C98(u8* a, f32* pos);                                 // 0x800C7C98
void  fn_800C94B0(u8* p, u8* item);                                 // 0x800C94B0
void  fn_800C8320(u8* a);                                           // 0x800C8320
u8*   fn_800C7B84(u8* a, f32* pos);                                 // 0x800C7B84
u32   fn_800C8334(u8* a, u8* obj, f32* pos);                        // 0x800C8334
u8*   fn_800C7A70(u8* a, f32* pos);                                 // 0x800C7A70
s32   fn_800C8468(u8* a, u8* obj, f32* p0, f32* p1);                // 0x800C8468
void  fn_800C8860(u8* a);                                           // 0x800C8860
f32   fn_800C88D0(u8* a);                                           // 0x800C88D0
void  fn_801C83AC(u8* mgr, u8* item);                               // 0x801C83AC
void  operator_delete(void* p);                                     // 0x801D426C
void  fn_800D795C(u8* tbl, s32 idx);                                // 0x800D795C
s32   fn_8001AFB4(u8* p, s32 del);                                  // 0x8001AFB4
u8*   fn_800D8724(u8* a, u32 key);                                  // 0x800D8724
Vec3  QuatGetAxisY(const Quat* q);                                  // 0x801452A8
void  QuatGetForward(Vec3* out, const Quat* q);                     // 0x801454CC
void  fn_801455E0(f32* out, u8* quat);                     // 0x801455E0
s32   fn_800DE678(u8* a, f32* pos, f32* out, f32 f1);              // 0x800DE678
s32   WorldGetPhase(void* world);                                   // 0x800D83A8
s32   fn_800517C0(void);                                            // 0x800517C0
void  fn_800E6468(void* rec, f32 dt);                               // 0x800E6468
s32   fn_80109A0C(s32 id);                                            // 0x80109A0C
s32   fn_80109600(void* p);                                         // 0x80109600
s32   fn_801098DC(s32 a0);                                          // 0x801098DC
BOOL  fn_80198C34(void* set, void* w);                              // 0x80198C34
void* ScreenSetFindWidget(void* set, u32 hash);                     // 0x80198E34
void  TextureRelease(void* tex);                                    // 0x80184028
void  fn_800E6A84(s32 arg0);                                        // 0x800E6A84
// PROTOS-END
}

// C++ linkage, as defined in small_8009c.cpp.
s32 RobotEvolutionExperiment_RobotGetLastRandom(u8* obj);
s32 RobotEvolutionExperiment_RobotIsLeft(u8* obj);
s32 RobotEvolutionExperiment_RobotIsCenter(u8* obj);
s32 RobotEvolutionExperiment_RobotIsRight(u8* obj);
s32 RobotEvolutionExperiment_RobotTaunts(u8* obj);
s32 RobotEvolutionExperiment_RobotFleesLeft(u8* obj);
s32 RobotEvolutionExperiment_RobotFleesRight(u8* obj);
s32 RobotEvolutionExperiment_RobotGameWonCheer(u8* obj);
s32 RobotEvolutionExperiment_RobotGameLostFlee(u8* obj);
s32 RobotEvolutionExperiment_RobotIsCorrectRobot(u8* obj);

extern s32 gScreenHeight;         // 0x805F938C
extern s32 gScreenWidth;          // 0x805F9388
extern "C++" void* gScreenSets;   // 0x805FB654
extern void* lbl_805FAFB0;        // 0x805FAFB0 (digging dirt-fling object)
extern void* lbl_805FAFB8;        // 0x805FAFB8 (puzzle object)
extern "C++" void* gEventMgr;     // 0x805FB6C8
extern u8 lbl_8047C760[];         // 0x8047C760 (effect manager)
extern void* lbl_805FBBAC;        // 0x805FBBAC (voice SFX handle)
extern s32 lbl_80347BF8[];        // 0x80347BF8 (+0: keys[10], +0x78: sound B[10], +0xF0: sound A[10])
extern void* lbl_805FAFC0;        // 0x805FAFC0 (bug-catching object)
extern f32 lbl_805F95A0;          // 0x805F95A0 (time scale)
extern u8 lbl_8035BDF0[];         // 0x8035BDF0 (robot game script name table)
extern void* lbl_805FB188;        // 0x805FB188 (spawn record table)
extern void* lbl_805FB1C8;        // 0x805FB1C8 (state record pointer)
extern u8* gWorldState;           // 0x803E6D00 (first word: world block pointer)
extern u8 gGame[];                // 0x803B53A0
extern u8 lbl_803C4298[];         // 0x803C4298
extern u8 gIgcMode;               // 0x805FAF64
extern const char lbl_80357368[]; // 0x80357368 (animation name string)

typedef void (*UIVFn)(u8* self, s32 arg);

// Calls slot 0x2C of the table pointed to by obj+0x138.
static inline void CallSlot2C(u8* obj, s32 arg) {
    (*(UIVFn*)(*(u8**)(obj + 0x138) + 0x2c))(obj, arg);
}

// Puzzle piece alignment test: not locked, and within 40 units of its target and 0.3 rad of upright.
static inline s32 PuzzlePieceAligned(u8* piece) {
    s32 r5 = 0, r6 = 0, r7 = 0;
    if (*(u8*)(piece + 0x44) == 0 && fabsf(*(f32*)(*(u8**)piece + 0x68) - *(f32*)(piece + 0x4)) < 40.0f) r7 = 1;
    if (r7 != 0 && fabsf(*(f32*)(*(u8**)piece + 0x6c) - *(f32*)(piece + 0x8)) < 40.0f) r6 = 1;
    if (r6 != 0 && fabsf(*(f32*)(*(u8**)piece + 0x98)) < 0.3f) r5 = 1;
    return r5;
}

extern "C" {

// 0x80024FA0
// Starts the dirt-fling element and plays its fall animation.
void DiggingPlayDirtFallingAnim(void) {
    f32 sp48[3];
    f32 sp38[3];
    u8* g = (u8*)lbl_805FAFB0;
    f32 a = *(f32*)(g + 0xaac);
    f32 b = *(f32*)(g + 0xab0);
    f32 hw = 0.5f * (f32)gScreenWidth;
    f32 hh = 0.5f * (f32)gScreenHeight;
    f32 x = hw + hw * (*(f32*)(g + 0xacc) * a);
    f32 y = hh + hh * (*(f32*)(g + 0xad0) * b);
    sp48[0] = x;
    sp48[1] = y;
    sp48[2] = 0.0f;
    sp38[0] = 0.0f;
    sp38[1] = 100.0f;
    sp38[2] = 0.0f;
    fn_80025524(g, sp48, sp38);
    void* gs = gScreenSets;
    u32 elem = StringHash("dirt_fling");
    u8* el = (u8*)ScreenSetFindElement(ScreenSetsCurrent(gs), elem);
    CallSlot2C(el, 2);
    u32 anim = StringHash("dirt_fling_anim");
    UIElement_PlayUIAnim(StringHash("dirt_fling"), anim);
}

// 0x80025DB4
// Picks the unplaced piece whose slot is near the cursor; returns whether a piece is held.
s32 PuzzleSelectPiece(void) {
    u8* g = (u8*)lbl_805FAFB8;
    f32 hw = 0.5f * (f32)gScreenWidth;
    f32 hh = 0.5f * (f32)gScreenHeight;
    f32 kx = *(f32*)(g + 0x17c) * *(f32*)(g + 0x15c);
    f32 ky = *(f32*)(g + 0x180) * *(f32*)(g + 0x160);
    f32 cx = hw + hw * kx;
    f32 cy = hh + hh * ky;
    for (s32 i = 0; i < 8; i++) {
        u8* piece = *(u8**)((u8*)lbl_805FAFB8 + 0xb0) + i * 0x48;
        f32 dx = -10.0f + (*(f32*)(piece + 0x1c) - cx);
        f32 dy = -10.0f + (*(f32*)(piece + 0x20) - cy);
        if (2500.0f >= dx * dx + dy * dy) {
            if (PuzzlePieceAligned(piece) == 0) {
                u8* p = (u8*)lbl_805FAFB8;
                *(s32*)(p + 0x98) = i;
                p = (u8*)lbl_805FAFB8;
                *(f32*)(p + 0x9c) = 0.0f;
                *(f32*)(p + 0xa0) = 0.0f;
                *(f32*)(p + 0xa4) = 0.0f;
                p = (u8*)lbl_805FAFB8;
                *(f32*)(p + 0xa8) = *(f32*)(p + 0x168) - *(f32*)(*(u8**)(*(u8**)(p + 0xb0) + i * 0x48) + 0x98);
                p = (u8*)lbl_805FAFB8;
                u8* sel = *(u8**)(p + 0xb0) + *(s32*)(p + 0x98) * 0x48;
                f32 f2 = *(f32*)(sel + 0x10);
                f32 f1 = *(f32*)(sel + 0x14);
                f32 f0 = *(f32*)(sel + 0x18);
                u8* obj = *(u8**)sel;
                *(f32*)(obj + 0x74) = f2;
                *(f32*)(obj + 0x78) = f1;
                *(f32*)(obj + 0x7c) = f0;
            }
        }
    }
    return *(s32*)((u8*)lbl_805FAFB8 + 0x98) >= 0;
}

// 0x8002605C
// Moves the cursor object to the screen position of the cursor and refreshes it.
void PuzzleRepositionCursor(void) {
    u8* g = (u8*)lbl_805FAFB8;
    f32 hh = 0.5f * (f32)gScreenHeight;
    f32 hw = 0.5f * (f32)gScreenWidth;
    f32 kx = *(f32*)(g + 0x17c) * *(f32*)(g + 0x15c);
    f32 ky = *(f32*)(g + 0x180) * *(f32*)(g + 0x160);
    f32 cy = hh + hh * ky;
    f32 cx = hw + hw * kx;
    f32 ang = *(f32*)(g + 0x168);
    u8* cur = *(u8**)(g + 0xb4);
    *(f32*)(cur + 0x68) = cx;
    *(f32*)(cur + 0x6c) = cy;
    g = (u8*)lbl_805FAFB8;
    if (*(s32*)(g + 0x98) > -1) {
        *(f32*)(*(u8**)(g + 0xb4) + 0x98) = ang;
    }
    cur = *(u8**)((u8*)lbl_805FAFB8 + 0xb4);
    CallSlot2C(cur, 0x1a);
}

// 0x80026194
// Drags the held piece to the cursor (minus grab offset) and wraps its angle to [-pi, pi].
void PuzzleMovePiece(void) {
    u8* g = (u8*)lbl_805FAFB8;
    f32 hh = 0.5f * (f32)gScreenHeight;
    f32 hw = 0.5f * (f32)gScreenWidth;
    f32 kx = *(f32*)(g + 0x17c) * *(f32*)(g + 0x15c);
    f32 ky = *(f32*)(g + 0x180) * *(f32*)(g + 0x160);
    f32 offX = *(f32*)(g + 0x9c);
    f32 offY = *(f32*)(g + 0xa0);
    f32 cy = hh + hh * ky;
    f32 cx = hw + hw * kx;
    f32 px = cx - offX;
    f32 py = cy - offY;
    f32 pz = 15.0f;
    f32 ang = *(f32*)(g + 0x168) - *(f32*)(g + 0xa8);
    while (ang > 3.1415927f) ang -= 6.2831855f;
    while (ang < -3.1415927f) ang += 6.2831855f;
    g = (u8*)lbl_805FAFB8;
    u8* pieces = *(u8**)(g + 0xb0);
    s32 off = *(s32*)(g + 0x98) * 0x48;
    *(f32*)(*(u8**)(pieces + off) + 0x68) = px;
    *(f32*)(*(u8**)(pieces + off) + 0x6c) = py;
    *(f32*)(*(u8**)(pieces + off) + 0x70) = pz;
    *(f32*)(*(u8**)(pieces + off) + 0x98) = ang;
    CallSlot2C(*(u8**)(pieces + off), 0x18);
}

// 0x80026378
// Drops the held piece: snaps it into place if aligned (returns 2 when all pieces are placed, else 1),
// otherwise returns it to its start pose (returns 0).
s32 PuzzlePiecePlaced(void) {
    u8* g = (u8*)lbl_805FAFB8;
    u8* piece = *(u8**)(g + 0xb0) + *(s32*)(g + 0x98) * 0x48;
    s32 aligned = PuzzlePieceAligned(piece);
    *(s32*)(g + 0x98) = -1;
    if (aligned != 0) {
        fn_8019CAD4(*(void**)((u8*)lbl_805FAFB8 + 0xc8), *(u8**)piece);
        *(f32*)(*(u8**)piece + 0x68) = *(f32*)(piece + 0x4);
        *(f32*)(*(u8**)piece + 0x6c) = *(f32*)(piece + 0x8);
        *(f32*)(*(u8**)piece + 0x70) = *(f32*)(piece + 0xc);
        *(f32*)(*(u8**)piece + 0x98) = 0.0f;
        CallSlot2C(*(u8**)piece, 0x18);
        CallSlot2C(*(u8**)((u8*)lbl_805FAFB8 + 0xc8), 0x1a);
        *(f32*)(*(u8**)((u8*)lbl_805FAFB8 + 0xc8) + 0x70) = 0.065f;
        UIWidgetPlayAnimLoop(*(void**)((u8*)lbl_805FAFB8 + 0xc8), StringHash(lbl_80357368), 10);
        u8* pieces = *(u8**)((u8*)lbl_805FAFB8 + 0xb0);
        s32 allDone = 1;
        for (s32 i = 0; i < 8; i++) {
            u8* p = pieces + i * 0x48;
            if (*(u8*)(p + 0x44) != 0) continue;
            if (PuzzlePieceAligned(p) == 0) {
                allDone = 0;
                break;
            }
        }
        return (allDone != 0) + 1;
    }
    f32 ang = *(f32*)(piece + 0x34);
    *(f32*)(*(u8**)piece + 0x68) = *(f32*)(piece + 0x1c);
    *(f32*)(*(u8**)piece + 0x6c) = *(f32*)(piece + 0x20);
    *(f32*)(*(u8**)piece + 0x70) = *(f32*)(piece + 0x24);
    *(f32*)(*(u8**)piece + 0x98) = ang;
    CallSlot2C(*(u8**)piece, 0x18);
    f32 f2 = *(f32*)(piece + 0x38);
    f32 f1 = *(f32*)(piece + 0x3c);
    f32 f0 = *(f32*)(piece + 0x40);
    u8* obj = *(u8**)piece;
    *(f32*)(obj + 0x74) = f2;
    *(f32*)(obj + 0x78) = f1;
    *(f32*)(obj + 0x7c) = f0;
    return 0;
}

// 0x80091AE0
// Smasher hit: for the slot whose part pointer matches, sends the hit event and spawns the hit effect.
void RobotEvolutionExperiment_IntroSmasherHit(u8* arg0) {
    u8* hit = fn_80093CF0(*(u8**)(arg0 + 0x10));
    if (hit == NULL) return;
    u8* obj = fn_8006FBDC();
    if (obj == NULL) return;
    if (fn_8006FBFC(obj) != 4) return;
    u8* base = obj;
    u8* a = obj + 0x180;
    u8* b = obj + 0xa8;
    f32 w = 0.0f;
    for (s32 i = 0; i < 3; i++) {
        if (a == hit) {
            EventSend(gEventMgr, *(void**)(obj + 0xac), NULL, 0xD7A63AA2u, 0, 0, 1, 0);
            fn_800928F4(b);
            u8* src = *(u8**)(base + 0x290);
            f32 pos[4];
            pos[0] = *(f32*)(src + 0x8);
            pos[1] = *(f32*)(src + 0xc);
            pos[2] = *(f32*)(src + 0x10);
            pos[3] = w;
            SpawnEffectAt(lbl_8047C760, 0xF7A1EDD6u, pos, 0);
        }
        obj += 0x48;
        b += 0x48;
        base += 4;
        a += 0x28;
    }
}

// 0x800775A4
// Looks up the key at arg1+1 in the 10-entry table and plays its two sounds, storing the handles.
void fn_800775A4(u8* arg0, u8* arg1) {
    s32 soundA = 0;
    s32 soundB = 0;
    for (s32 i = 0; i < 10; i++) {
        if ((s32)*(u8*)(arg1 + 1) == lbl_80347BF8[i]) {
            soundA = lbl_80347BF8[0x3C + i];
            soundB = lbl_80347BF8[0x1E + i];
            break;
        }
    }
    if (soundA != 0 && soundB != 0) {
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) v = fn_80209814(v);
            lbl_805FBBAC = v;
        }
        *(s32*)(arg0 + 0x12e4) = PlaySound((u32)soundA, 0, 0, 4, 0, 0);
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) v = fn_80209814(v);
            lbl_805FBBAC = v;
        }
        *(s32*)(arg0 + 0x12e8) = PlaySound((u32)soundB, 0, 0, 5, 0, 0);
    }
}

// 0x8007AE90
// Computes the bounds of arg3 and the two arg0 objects, tests them with fn_8007AC80 and, if hit,
// builds and applies the result. Returns whether it hit.
s32 fn_8007AE90(u8* arg0, u8* arg1, u8* arg2, u8* arg3) {
    f32 s28, s2c, s30, s34;
    f32 s40[2], s38[2], s50[2], s48[2];
    f32 s20[2], s18[2], s10[2], s8[2];
    u8 s58[0x40];
    fn_800D8CB0(arg3, &s28, &s2c, &s30, &s34);
    fn_8007C5B4(*(u8**)arg0, (u8*)s40, (u8*)s38);
    fn_8007A3F4(*(u8**)(arg0 + 4), (u8*)s50, (u8*)s48);
    s20[0] = s50[0];
    s20[1] = s50[1];
    s18[0] = s48[0];
    s18[1] = s48[1];
    s10[0] = s40[0];
    s10[1] = s40[1];
    s8[0] = s38[0];
    s8[1] = s38[1];
    if (fn_8007AC80(arg0, s20, s18, s10, s8, &s30, &s28) == 0) return 0;
    fn_8007A8D0(arg0, s58, s20, s18, s10, s8);
    fn_8007A68C(arg0, s58, arg1, arg2);
    return 1;
}

// 0x8007C958
// Places the two attached objects at arg1 plus their offsets (flagging them dirty), then rebuilds
// the box at +0x28..+0x34 from the second object's bounds shifted by its position.
void fn_8007C958(u8* arg0, f32* arg1) {
    u8* a = *(u8**)arg0;
    if (a != NULL) {
        f32 x = arg1[0] + *(f32*)(arg0 + 0xc);
        f32 y = arg1[1] + *(f32*)(arg0 + 0x10);
        f32 z = arg1[2] + *(f32*)(arg0 + 0x14);
        u8* in = *(u8**)(a + 4);
        *(f32*)(in + 0x0) = x;
        *(f32*)(in + 0x4) = y;
        *(f32*)(in + 0x8) = z;
        *(u32*)(*(u8**)(a + 4) + 0x20) |= 0x8000;
        *(u32*)(*(u8**)(a + 4) + 0x20) |= 0x10000000;
        *(u32*)(*(u8**)(a + 4) + 0x20) |= 0x4000;
        *(u32*)(*(u8**)(a + 4) + 0x20) |= 0x10000000;
        fn_801B4EA4(a, 0);
    }
    u8* b = *(u8**)(arg0 + 4);
    {
        f32 x = arg1[0] + *(f32*)(arg0 + 0x18);
        f32 y = arg1[1] + *(f32*)(arg0 + 0x1c);
        f32 z = arg1[2] + *(f32*)(arg0 + 0x20);
        u8* in = *(u8**)(b + 4);
        *(f32*)(in + 0x0) = x;
        *(f32*)(in + 0x4) = y;
        *(f32*)(in + 0x8) = z;
        *(u32*)(*(u8**)(b + 4) + 0x20) |= 0x8000;
        *(u32*)(*(u8**)(b + 4) + 0x20) |= 0x10000000;
        *(u32*)(*(u8**)(b + 4) + 0x20) |= 0x4000;
        *(u32*)(*(u8**)(b + 4) + 0x20) |= 0x10000000;
        fn_801B4EA4(b, 0);
    }
    u8* bb = *(u8**)(arg0 + 4);
    typedef void* (*GetFn)(u8*);
    u8* bounds = *(u8**)((u8*)fn_801DA758((*(GetFn*)(*(u8**)bb + 0xc))(bb)) + 0x10);
    f32 b30 = *(f32*)(bounds + 0x30);
    f32 b38 = *(f32*)(bounds + 0x38);
    f32 b3c = *(f32*)(bounds + 0x3c);
    f32 b44 = *(f32*)(bounds + 0x44);
    u8* pos = *(u8**)(*(u8**)(arg0 + 4) + 4);
    f32 px = *(f32*)(pos + 0x0);
    f32 pz = *(f32*)(pos + 0x8);
    *(f32*)(arg0 + 0x28) = b30 + px;
    *(f32*)(arg0 + 0x2c) = b38 + pz;
    *(f32*)(arg0 + 0x30) = b3c + px;
    *(f32*)(arg0 + 0x34) = b44 + pz;
}

// 0x800291C8
// Bug-catching sound/state step: tracks the deltas of two counters, plays a sound for a catch and
// returns a state code (0..3 from the first delta, 4 on a third consecutive tick, 5/6 on catches).
s32 CheckBugSound(void) {
    u8* g = (u8*)lbl_805FAFC0;
    *(u8*)(g + 0xfd) = 1;
    g = (u8*)lbl_805FAFC0;
    s32 c0 = *(s32*)(g + 0xec);
    s32 prev0 = *(s32*)(g + 0x114);
    s32 prev1 = *(s32*)(g + 0x118);
    s32 c1 = *(s32*)(g + 0xf0);
    s32 d0 = c0 - prev0;
    *(s32*)(g + 0x114) = c0;
    s32 d1 = c1 - prev1;
    g = (u8*)lbl_805FAFC0;
    *(s32*)(g + 0x118) = *(s32*)(g + 0xf0);
    if (d0 > 0 && d1 == 0) {
        g = (u8*)lbl_805FAFC0;
        *(s32*)(g + 0xf4) = *(s32*)(g + 0xf4) + 1;
        g = (u8*)lbl_805FAFC0;
        if (*(s32*)(g + 0xf4) > 2) {
            *(s32*)(g + 0xf4) = 0;
            return 4;
        }
    } else {
        *(s32*)((u8*)lbl_805FAFC0 + 0xf4) = 0;
    }
    if (d1 == 1) {
        PlaySound(0xC1705E2Du, 0, 0, 0, 0, 0);
    } else if (d1 > 1) {
        PlaySound(0xC93756EAu, 0, 0, 0, 0, 0);
    }
    if (d1 <= 0) {
        if (d0 == 3) return 2;
        if (d0 > 3) return 3;
        if (d0 == 0) return 0;
        if (d0 > 0) return 1;
        return 3;
    }
    return (d1 == 1) ? 5 : 6;
}

// 0x801458B0
// Builds a quaternion (out) from an orthonormal frame: A = norm(arg1 x arg2), B = norm(A x arg1).
void fn_801458B0(f32* arg0, f32* arg1, f32* arg2) {
    Vec3 v1 = { arg1[0], arg1[1], arg1[2] };
    Vec3 c = {
        arg1[1] * arg2[2] - arg1[2] * arg2[1],
        arg1[2] * arg2[0] - arg1[0] * arg2[2],
        arg1[0] * arg2[1] - arg1[1] * arg2[0],
    };
    Vec3 a = Vec3Normalized(c);
    Vec3 c2 = {
        a.y * v1.z - a.z * v1.y,
        a.z * v1.x - a.x * v1.z,
        a.x * v1.y - a.y * v1.x,
    };
    Vec3 b = Vec3Normalized(c2);
    f32 m[12] = { a.x, a.y, a.z, v1.x, v1.y, v1.z, b.x, b.y, b.z, 0.0f, 0.0f, 0.0f };
    QuatFromMtx((Quat*)arg0, m);
}

// 0x801C5CC0
// Moves a tracked pair of points to new positions (arg1, arg2), keeping the previous ones at
// +0x44/+0x5C; under slow motion (time scale < 1) the previous points are extrapolated back.
void fn_801C5CC0(u8* arg0, f32* arg1, f32* arg2) {
    s32 nearZero = 0;
    u8* src = *(u8**)arg0;
    *(f32*)(arg0 + 0x38) = *(f32*)(src + 0x38);
    *(f32*)(arg0 + 0x50) = *(f32*)(src + 0x38);
    if (lbl_805F95A0 > -0.0001f && lbl_805F95A0 < 0.0001f) nearZero = 1;
    if (nearZero != 0) return;
    f32 ox = *(f32*)(arg0 + 0x14);
    f32 oy = *(f32*)(arg0 + 0x18);
    f32 oz = *(f32*)(arg0 + 0x1c);
    f32 px = *(f32*)(arg0 + 0x2c);
    f32 py = *(f32*)(arg0 + 0x30);
    f32 pz = *(f32*)(arg0 + 0x34);
    *(f32*)(arg0 + 0x44) = ox;
    *(f32*)(arg0 + 0x48) = oy;
    *(f32*)(arg0 + 0x4c) = oz;
    *(f32*)(arg0 + 0x5c) = px;
    *(f32*)(arg0 + 0x60) = py;
    *(f32*)(arg0 + 0x64) = pz;
    *(f32*)(arg0 + 0x14) = arg1[0];
    *(f32*)(arg0 + 0x18) = arg1[1];
    *(f32*)(arg0 + 0x1c) = arg1[2];
    *(f32*)(arg0 + 0x2c) = arg2[0];
    *(f32*)(arg0 + 0x30) = arg2[1];
    *(f32*)(arg0 + 0x34) = arg2[2];
    if (lbl_805F95A0 < 1.0f) {
        f32 inv = 1.0f / lbl_805F95A0;
        f32 dx = (ox - arg1[0]) * inv;
        f32 dy = (oy - arg1[1]) * inv;
        f32 dz = (oz - arg1[2]) * inv;
        *(f32*)(arg0 + 0x44) = arg1[0] + dx;
        *(f32*)(arg0 + 0x48) = arg1[1] + dy;
        *(f32*)(arg0 + 0x4c) = arg1[2] + dz;
        *(f32*)(arg0 + 0x5c) = arg2[0] + dx;
        *(f32*)(arg0 + 0x60) = arg2[1] + dy;
        *(f32*)(arg0 + 0x64) = arg2[2] + dz;
    }
}

// 0x800921B0
// Registers the robot-game script callbacks (events 0x4E, queries 0x1D).
void RobotEvolutionExperiment_RegisterRobotScripts(void) {
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x00), 0x4E, (void*)RobotEvolutionExperiment_RobotReadyToLaunch);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x2C), 0x4E, (void*)RobotEvolutionExperiment_RobotLanded);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x54), 0x4E, (void*)RobotEvolutionExperiment_RobotReadyToPlay);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x80), 0x4E, (void*)RobotEvolutionExperiment_RobotShowAnswer);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0xAC), 0x4E, (void*)RobotEvolutionExperiment_RobotStartFleeCycle);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0xDC), 0x1D, (void*)RobotEvolutionExperiment_RobotGetLastRandom);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x108), 0x1D, (void*)RobotEvolutionExperiment_RobotIsLeft);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x130), 0x1D, (void*)RobotEvolutionExperiment_RobotIsCenter);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x158), 0x1D, (void*)RobotEvolutionExperiment_RobotIsRight);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x180), 0x1D, (void*)RobotEvolutionExperiment_RobotTaunts);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x1A8), 0x1D, (void*)RobotEvolutionExperiment_RobotFleesLeft);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x1D0), 0x1D, (void*)RobotEvolutionExperiment_RobotFleesRight);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x1FC), 0x1D, (void*)RobotEvolutionExperiment_RobotGameOver);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x224), 0x1D, (void*)RobotEvolutionExperiment_RobotGameWonCheer);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x250), 0x1D, (void*)RobotEvolutionExperiment_RobotGameLostFlee);
    RegisterScriptFunc((const char*)(lbl_8035BDF0 + 0x27C), 0x1D, (void*)RobotEvolutionExperiment_RobotIsCorrectRobot);
}

// 0x800B1D98
// Three-state hysteresis on the float at +0x268 (state at +0x26C): rising past 0.6/0.9 steps the
// state up, falling below 0.5/0.7 steps it down; each change sends an event to both objects.
void fn_800B1D98(u8* arg0) {
    s32 state = *(s32*)(arg0 + 0x26c);
    if (state == 0) {
        if (*(f32*)(arg0 + 0x268) >= 0.6f) {
            fn_800A7D88(arg0, 0x8EAA2203u);
            fn_800A7D88(*(void**)(arg0 + 0x264), 0x8EAA2203u);
            *(s32*)(arg0 + 0x26c) = 1;
        }
    } else if (state == 1) {
        f32 v = *(f32*)(arg0 + 0x268);
        if (v >= 0.9f) {
            fn_800A7D88(arg0, 0x8EAA2203u);
            fn_800A7D88(*(void**)(arg0 + 0x264), 0x8EAA2203u);
            *(s32*)(arg0 + 0x26c) = 2;
        } else if (v <= 0.5f) {
            fn_800A7D88(arg0, 0x36BCBA77u);
            fn_800A7D88(*(void**)(arg0 + 0x264), 0x36BCBA77u);
            *(s32*)(arg0 + 0x26c) = 0;
        }
    } else if (state == 2) {
        if (*(f32*)(arg0 + 0x268) <= 0.7f) {
            fn_800A7D88(arg0, 0x36BCBA77u);
            fn_800A7D88(*(void**)(arg0 + 0x264), 0x36BCBA77u);
            *(s32*)(arg0 + 0x26c) = 1;
        }
    }
}

// 0x800B60A0
// Resets the object: copies its position (+0x264..) into the node at +4, marks the node dirty and
// clears the motion fields.
void fn_800B60A0(u8* arg0) {
    u8* node = *(u8**)(arg0 + 0x4);
    *(f32*)(node + 0x0) = *(f32*)(arg0 + 0x264);
    *(f32*)(node + 0x4) = *(f32*)(arg0 + 0x268);
    *(f32*)(node + 0x8) = *(f32*)(arg0 + 0x26c);
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    *(f32*)(arg0 + 0x27c) = 0.0f;
    node = *(u8**)(arg0 + 0x4);
    *(f32*)(arg0 + 0x280) = 0.0f;
    *(f32*)(arg0 + 0x284) = 0.0f;
    *(f32*)(arg0 + 0x288) = 1.0f;
    *(f32*)(arg0 + 0x2c4) = 0.0f;
    *(f32*)(arg0 + 0x2cc) = 0.0f;
    *(f32*)(arg0 + 0x2d0) = 0.0f;
    *(s32*)(arg0 + 0x2b0) = 0;
    *(s32*)(arg0 + 0x2b8) = -1;
    *(s32*)(arg0 + 0x2c0) = -1;
    *(s32*)(arg0 + 0x2f8) = -1;
    *(s32*)(arg0 + 0x2fc) = -1;
    *(s32*)(arg0 + 0x300) = -1;
    *(s32*)(arg0 + 0x2b4) = 0;
    *(f32*)(node + 0xc) = 0.0f;
    *(f32*)(node + 0x10) = *(f32*)(arg0 + 0x280);
    *(f32*)(node + 0x14) = *(f32*)(arg0 + 0x284);
    *(f32*)(node + 0x18) = *(f32*)(arg0 + 0x288);
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x8000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x4000;
    *(u32*)(*(u8**)(arg0 + 4) + 0x20) |= 0x10000000;
}

// 0x800C8118
// Prepares the path/ray state for object arg1, then steps it until fn_800C8334 stops returning 1,
// flagging *arg2 if any step hit; returns fn_800C88D0's result, or -1 if arg1 has no object.
f32 fn_800C8118(u8* arg0, u8* arg1, u8* arg2) {
    f32 pos[3];
    f32 box[4];
    u8* obj = fn_800C73EC(arg1);
    if (obj == NULL) return -1.0f;
    u8* d = *(u8**)(obj + 4);
    pos[0] = *(f32*)(d + 0x0);
    pos[1] = *(f32*)(d + 0x4);
    pos[2] = *(f32*)(d + 0x8);
    d = *(u8**)(obj + 4);
    box[0] = *(f32*)(d + 0xc);
    box[1] = *(f32*)(d + 0x10);
    box[2] = *(f32*)(d + 0x14);
    box[3] = *(f32*)(d + 0x18);
    u8* head = fn_800C7C98(arg0, pos);
    fn_800C94B0(head, arg1);
    fn_800C8320(arg0);
    u32 r = 1;
    while (r == 1) {
        u8* t = fn_800C7B84(arg0, pos);
        f32 p2[3];
        p2[0] = pos[0];
        p2[1] = pos[1];
        p2[2] = pos[2];
        r = fn_800C8334(arg0, t, p2);
        if (r == 1) *(u8*)arg2 = 1;
    }
    u8* t2 = fn_800C7A70(arg0, pos);
    f32 b8[4] = { box[0], box[1], box[2], box[3] };
    f32 p18[3] = { pos[0], pos[1], pos[2] };
    if (fn_800C8468(arg0, t2, p18, b8) == 0 && *(u8*)arg2 == 0) {
        fn_800C8860(arg0);
    }
    return fn_800C88D0(arg0);
}

// 0x800D59F4
// Sets the byte flag at +0x9C of the attached object; turning it on spawns the effect for its
// kind (4 or 0x10) at its position with the object as owner, turning it off releases the effect.
void fn_800D59F4(u8* arg0, s32 arg1) {
    u8* obj = *(u8**)(*(u8**)arg0 + 0x14);
    if (obj == NULL) return;
    *(u8*)(obj + 0x9c) = (u8)arg1;
    obj = *(u8**)(*(u8**)arg0 + 0x14);
    if (obj == NULL) return;
    if (arg1 != 0 && *(s32*)(obj + 0x50) == 0) {
        s32 kind = *(s32*)(obj + 0x40);
        f32 pos[4];
        if (kind == 0x10) {
            pos[0] = *(f32*)(obj + 0x34);
            pos[1] = *(f32*)(obj + 0x38);
            pos[2] = *(f32*)(obj + 0x3c);
            pos[3] = 0.0f;
            SpawnEffectAt(lbl_8047C760, 0xE8774AA7u, pos, (u8**)(obj + 0x50));
        } else if (kind == 4) {
            pos[0] = *(f32*)(obj + 0x34);
            pos[1] = *(f32*)(obj + 0x38);
            pos[2] = *(f32*)(obj + 0x3c);
            pos[3] = 0.0f;
            SpawnEffectAt(lbl_8047C760, 0x8BEBBC3Du, pos, (u8**)(obj + 0x50));
        }
    } else if (arg1 == 0) {
        u8* fx = *(u8**)(obj + 0x50);
        if (fx != NULL) {
            fn_801C83AC(lbl_8047C760, fx);
            *(u8**)(*(u8**)(*(u8**)arg0 + 0x14) + 0x50) = NULL;
        }
    }
}

// 0x800D61D8
// Depth-first search through a node graph (nodes have 4 edge slots at +4, edge flags at +0x24).
// Returns the slot index of the first edge on a path to an edge whose id (+0) equals target, else -1.
// arg0 holds a visited list: count at +0x400, entries from +4.
s32 fn_800D61D8(u8* arg0, u8* arg1, u32 arg2, s32 arg3) {
    if (arg1 == NULL || arg2 == 0) return -1;
    u32 mask = 0;
    if (*(s32*)(arg1 + 0x4) != 0 && (*(s32*)(arg1 + 0x24) == 0 || arg3 != 0)) mask = 1u << 0;
    if (*(s32*)(arg1 + 0x8) != 0 && (*(s32*)(arg1 + 0x28) == 0 || arg3 != 0)) mask |= 1u << 1;
    if (*(s32*)(arg1 + 0xc) != 0 && (*(s32*)(arg1 + 0x2c) == 0 || arg3 != 0)) mask |= 1u << 2;
    if (*(s32*)(arg1 + 0x10) != 0 && (*(s32*)(arg1 + 0x30) == 0 || arg3 != 0)) mask |= 1u << 3;
    u8* slot = arg1;
    for (s32 i = 0; i < 4; i++, slot += 4) {
        if (!(mask & (1u << i))) continue;
        u8* other = NULL;
        u8* edge = *(u8**)(slot + 4);
        if (edge != NULL) {
            other = *(u8**)(edge + 4);
            if (arg1 == other) other = *(u8**)(edge + 8);
        }
        if (other == NULL) continue;
        if (arg2 == *(u32*)(*(u8**)(slot + 4))) return i;
        s32 count = *(s32*)(arg0 + 0x400);
        s32 seen = 0;
        for (s32 k = 0; k < count; k++) {
            if (*(u8**)(arg0 + 4 + k * 4) == other) { seen = 1; break; }
        }
        if (seen) continue;
        *(u8**)(arg0 + 4 + count * 4) = other;
        *(s32*)(arg0 + 0x400) = *(s32*)(arg0 + 0x400) + 1;
        if (fn_800D61D8(arg0, other, arg2, arg3) != -1) return i;
    }
    return -1;
}

// Index accessor of the spawn record table: out-of-range indexes give the table base.
static inline u8* SpawnRecAt(u8* tbl, s32 count, s32 i) {
    return (i < 0 || i > count) ? tbl : tbl + i * 0x28;
}

// 0x800D6B44
// (Re)creates the 20-entry spawn record table (0x28-byte records, count at +0x320) at lbl_805FB188.
u8* fn_800D6B44(void) {
    if (lbl_805FB188 != NULL) operator_delete(lbl_805FB188);
    lbl_805FB188 = NULL;
    u8* t = (u8*)fn_801D40E0(0x324);
    if (t != NULL) {
        for (s32 i = 0; i < 20; i++) {
            u8* r = t + i * 0x28;
            *(s32*)(r + 0x0) = 0;
            *(s32*)(r + 0x4) = 0;
            *(s32*)(r + 0x8) = 0;
            *(f32*)(r + 0xc) = 0.0f;
            *(f32*)(r + 0x10) = 0.0f;
            *(f32*)(r + 0x14) = 0.0f;
            *(s32*)(r + 0x18) = -1;
            *(s32*)(r + 0x1c) = -1;
            *(u8*)(r + 0x20) = 0;
            *(s32*)(r + 0x24) = 0;
        }
        *(s32*)(t + 0x320) = 0;
    }
    lbl_805FB188 = t;
    *(s32*)(t + 0x320) = 0;
    return t;
}

// 0x800D7288
// Removes the spawn record whose first word equals handle, moving the last record into its slot.
void fn_800D7288(s32 handle) {
    u8* tbl = (u8*)lbl_805FB188;
    if (tbl == NULL) return;
    s32 found = -1;
    s32 n = *(s32*)(tbl + 0x320);
    for (s32 i = 0; i < n; i++) {
        u8* r = SpawnRecAt(tbl, *(s32*)((u8*)lbl_805FB188 + 0x320), i);
        if ((u32)handle == *(u32*)r) { found = i; break; }
    }
    if (found < 0) return;
    u8* t = (u8*)lbl_805FB188;
    s32 count = *(s32*)(t + 0x320);
    if (found >= count) return;
    s32 last = count - 1;
    *(s32*)(t + 0x320) = last;
    if (found == last) return;
    u8* src = t + last * 0x28;
    u8* dst = t + found * 0x28;
    *(u32*)(dst + 0x0) = *(u32*)(src + 0x0);
    *(s32*)(dst + 0x4) = *(s32*)(src + 0x4);
    *(s32*)(dst + 0x8) = *(s32*)(src + 0x8);
    *(f32*)(dst + 0xc) = *(f32*)(src + 0xc);
    *(f32*)(dst + 0x10) = *(f32*)(src + 0x10);
    *(f32*)(dst + 0x14) = *(f32*)(src + 0x14);
    *(s32*)(dst + 0x18) = *(s32*)(src + 0x18);
    *(s32*)(dst + 0x1c) = *(s32*)(src + 0x1c);
    *(u8*)(dst + 0x20) = *(u8*)(src + 0x20);
    *(s32*)(dst + 0x24) = *(s32*)(src + 0x24);
}

// 0x800D75D0
// Finds the active spawn record (flag byte +0x20) whose owner (+0x24) is obj, clears it via
// fn_800D795C and zeroes its owner.
void fn_800D75D0(u8* arg0) {
    u8* tbl = (u8*)lbl_805FB188;
    if (tbl == NULL) return;
    s32 n = *(s32*)(tbl + 0x320);
    for (s32 i = 0; i < n; i++) {
        s32 count = *(s32*)(tbl + 0x320);
        u8* r = SpawnRecAt(tbl, count, i);
        if (*(u8*)(r + 0x20) != 0) {
            r = SpawnRecAt(tbl, count, i);
            if (*(u8**)(r + 0x24) == arg0) {
                fn_800D795C(tbl, i);
                u8* t2 = (u8*)lbl_805FB188;
                u8* r2 = SpawnRecAt(t2, *(s32*)(t2 + 0x320), i);
                *(s32*)(r2 + 0x24) = 0;
                return;
            }
        }
    }
}

// 0x800D7FF0
// Resets the object's selection fields; when arg1 is 0 re-selects the current slot from the world
// block (gWorldState +0x1E0/+0x1E8) via fn_800D8724.
void fn_800D7FF0(u8* arg0, s32 arg1) {
    *(s32*)(arg0 + 0x3c) = -1;
    *(s32*)(arg0 + 0x40) = -1;
    if (arg1 != 0) {
        u8* t = *(u8**)(arg0 + 0x44);
        if (t != NULL) {
            fn_8001AFB4(t, 0);
            t = *(u8**)(arg0 + 0x44);
            if (*(s32*)(t + 0x434) != 0) *(s32*)(t + 0x434) = 0;
        }
    }
    *(s32*)(arg0 + 0x4) = 0;
    *(s32*)(arg0 + 0x8) = 0;
    *(s32*)(arg0 + 0xc) = 0;
    *(s32*)(arg0 + 0x10) = 0;
    *(s32*)(arg0 + 0x14) = 0;
    *(s32*)(arg0 + 0x18) = 0;
    *(s32*)(arg0 + 0x1c) = 0;
    *(s32*)(arg0 + 0x20) = 0;
    *(s32*)(arg0 + 0x28) = 0;
    *(s32*)(arg0 + 0x2c) = 0;
    *(s32*)(arg0 + 0x24) = 0;
    *(s32*)(arg0 + 0x30) = 0;
    *(s32*)(arg0 + 0x0) = 0;
    *(s32*)(arg0 + 0x34) = 0;
    *(s32*)(arg0 + 0x38) = 0;
    if (arg1 == 0) {
        s32 idx = *(s32*)(gWorldState + 0x1e0);
        *(u8**)(arg0 + 0x24) = *(u8**)(arg0 + idx * 4 + 4);
        *(s32*)(arg0 + 0x2c) = 0;
        *(s32*)(arg0 + 0x28) = idx;
        u8* r = fn_800D8724(arg0, *(u32*)(gWorldState + 0x1e8));
        if (r != NULL) {
            *(u8**)(arg0 + 0x24) = r;
            *(u8**)(arg0 + *(s32*)(gWorldState + 0x1e0) * 4 + 4) = r;
        }
    }
}

// 0x800DB370
// Outputs the camera-style basis of the object's orientation (quat at +4 -> +0xC): arg2 = axis Y,
// arg3 = forward, arg1 = fn_801455E0 vector; identity-like defaults when there is no orientation.
void fn_800DB370(u8* arg0, f32* arg1, f32* arg2, f32* arg3) {
    if (*(u8**)(arg0 + 4) == NULL) {
        arg3[1] = 0.0f;
        arg3[0] = 1.0f;
        arg3[2] = 0.0f;
        arg1[0] = 0.0f;
        arg1[1] = 1.0f;
        arg1[2] = 0.0f;
        arg2[0] = 0.0f;
        arg2[1] = 0.0f;
        arg2[2] = 1.0f;
        return;
    }
    Vec3 axis = QuatGetAxisY((const Quat*)(*(u8**)(arg0 + 4) + 0xc));
    arg2[0] = axis.x;
    arg2[1] = axis.y;
    arg2[2] = axis.z;
    Vec3 fwd;
    QuatGetForward(&fwd, (const Quat*)(*(u8**)(arg0 + 4) + 0xc));
    arg3[0] = fwd.x;
    arg3[1] = fwd.y;
    arg3[2] = fwd.z;
    f32 v[3];
    fn_801455E0(v, *(u8**)(arg0 + 4) + 0xc);
    arg1[0] = v[0];
    arg1[1] = v[1];
    arg1[2] = v[2];
}

// 0x800DECD8
// Per-frame update of a timed blend: wraps the angle at +0x34 into [-pi, pi], counts down +0x3C by dt,
// and drives the blend factor at +0x44 from the timer at +0x50 (or the ratio +0x20/+0x48).
// Returns whether the blend factor is non-zero.
s32 fn_800DECD8(u8* arg0, f32 dt) {
    u8* cam = *(u8**)(lbl_803C4298 + 8);
    u8* sensor = *(u8**)(cam + 0x2c0);
    u8* posp = *(u8**)(sensor + 4);
    f32 pos[3];
    pos[0] = *(f32*)(posp + 0x0);
    pos[1] = *(f32*)(posp + 0x4);
    pos[2] = *(f32*)(posp + 0x8);
    *(u8*)(arg0 + 0x55) = (u8)fn_800DE678(arg0 + 0xc, pos, (f32*)(arg0 + 0x20), *(f32*)(sensor + 0x14));
    s32 blocked = 0;
    s32 timed = 0;
    if (*(f32*)(arg0 + 0x3c) > 0.0f && gIgcMode == 0) timed = 1;
    if (timed != 0) {
        s32 ok = 1;
        u32 t = *(u32*)(arg0 + 0x8) - 4;
        if (t <= 0xA && ((1u << t) & 0x423u)) ok = 0;
        if (ok != 0) blocked = 1;
    }
    f32 ang = *(f32*)(arg0 + 0x34);
    f32 a1;
    if (ang < -6.2831855f) a1 = -6.2831855f;
    else if (ang > 6.2831855f) a1 = 6.2831855f;
    else a1 = ang;
    if (a1 < -3.1415927f) a1 += 6.2831855f;
    else if (a1 > 3.1415927f) a1 -= 6.2831855f;
    f32 a2;
    if (a1 < -3.1415927f) a2 = -3.1415927f;
    else if (a1 > 3.1415927f) a2 = 3.1415927f;
    else a2 = a1;
    *(f32*)(arg0 + 0x34) = a2;
    *(f32*)(arg0 + 0x3c) = *(f32*)(arg0 + 0x3c) - dt;
    f32 dur = *(f32*)(arg0 + 0x4c);
    if (dur > 0.0f) {
        if (*(u8*)(arg0 + 0x54) != 0 && *(u8*)(arg0 + 0x55) != 0 && blocked == 0) {
            *(f32*)(arg0 + 0x50) = *(f32*)(arg0 + 0x50) + dt;
        } else {
            *(f32*)(arg0 + 0x50) = *(f32*)(arg0 + 0x50) - dt;
        }
        f32 c = 0.0f;
        f32 hi = *(f32*)(arg0 + 0x4c);
        f32 v = *(f32*)(arg0 + 0x50);
        if (0.0f < hi) {
            if (v < 0.0f) c = 0.0f;
            else c = (v > hi) ? hi : v;
        } else {
            if (v < hi) c = hi;
            else c = (v > 0.0f) ? 0.0f : v;
        }
        *(f32*)(arg0 + 0x50) = c;
        *(f32*)(arg0 + 0x44) = c / *(f32*)(arg0 + 0x4c);
    } else if (*(u8*)(arg0 + 0x54) != 0 && *(u8*)(arg0 + 0x55) != 0 && blocked == 0) {
        f32 ratio = *(f32*)(arg0 + 0x20) / *(f32*)(arg0 + 0x48);
        f32 f3 = 0.0f;
        if (ratio < 0.0f) f3 = 0.0f;
        else f3 = (ratio > 1.0f) ? 1.0f : ratio;
        *(f32*)(arg0 + 0x44) = f3;
    } else {
        *(f32*)(arg0 + 0x44) = 0.0f;
    }
    f32 blend = *(f32*)(arg0 + 0x44);
    s32 nearZero = 0;
    if (blend > -0.0001f && blend < 0.0001f) nearZero = 1;
    return nearZero == 0;
}

// 0x800E6360
// Per-frame hook: updates the state record at lbl_805FB1C8 while in world phase 1 or 2 (and idle),
// otherwise tears it down; then promotes a pending handle (+0xC) to current (+8) once it is ready.
void fn_800E6360(f32 dt) {
    s32 run = 0;
    if ((WorldGetPhase(gGame + 0xD5F0) == 1 || WorldGetPhase(gGame + 0xD5F0) == 2)
        && *(s32*)(lbl_803C4298 + 0x18) == 5
        && fn_800517C0() == 0
        && *(u8*)((u8*)lbl_805FB1C8 + 0x10) == 0) {
        run = 1;
    }
    if (run != 0) {
        fn_800E6468(lbl_805FB1C8, dt);
    } else {
        fn_800E6A84(0);
    }
    if (*(s32*)((u8*)lbl_805FB1C8 + 0xc) != 0 && fn_80109A0C(*(s32*)((u8*)lbl_805FB1C8 + 0xc)) == 0
        && fn_80109600((u8*)lbl_805FB1C8 + 0xc) != 0) {
        u8* r = (u8*)lbl_805FB1C8;
        *(s32*)(r + 0x8) = *(s32*)(r + 0xc);
        *(s32*)((u8*)lbl_805FB1C8 + 0xc) = 0;
    }
}

// 0x800E6A84
// Closes the popup screen if open, clears the flag bytes (+0x11, and +0x10 if arg0), drops the
// texture reference at +0 and releases the handle at +4.
void fn_800E6A84(s32 arg0) {
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        void* w = ScreenSetFindWidget(set, 0x353388F2u);
        if (w != NULL && fn_80198C34(set, w) != 0) {
            UIElement_Reset(0x24542991u);
            UIImage_SetTexture(0x5881687Au, 0);
            UIScreen_PopScreen(0x353388F2u);
        }
    }
    if (*(u8*)((u8*)lbl_805FB1C8 + 0x11) != 0) *(u8*)((u8*)lbl_805FB1C8 + 0x11) = 0;
    if (arg0 != 0) *(u8*)((u8*)lbl_805FB1C8 + 0x10) = 0;
    u8* tex = *(u8**)lbl_805FB1C8;
    if (tex != NULL) {
        s32 rc = *(s32*)(tex + 0x24) - 1;
        *(s32*)(tex + 0x24) = rc;
        if (rc <= 0 && rc == 0) TextureRelease(tex);
        *(u8**)lbl_805FB1C8 = NULL;
    }
    u8* rec = (u8*)lbl_805FB1C8;
    tex = *(u8**)rec;
    if (tex != NULL) {
        s32 rc = *(s32*)(tex + 0x24) - 1;
        *(s32*)(tex + 0x24) = rc;
        if (rc <= 0 && rc == 0) TextureRelease(tex);
        *(u8**)rec = NULL;
    }
    if (*(s32*)(rec + 0x4) != 0) {
        if (fn_80109A0C(*(s32*)(rec + 0x4)) == 0) fn_801098DC(*(s32*)(rec + 0x4));
        *(s32*)(rec + 0x4) = 0;
    }
}

// FUNCS-END
}
