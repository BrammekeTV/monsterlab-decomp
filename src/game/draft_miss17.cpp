// Small functions: combat HUD input toggles, character ground/flag helpers, collision checks,
// splash/books/monster cleanup, friend list floaty, name check (0x8001CD4C - 0x80140404).
#include "types.h"
#include <string.h>
extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };  // as in props.cpp

extern "C" {
void*  AnimSetGetClip(void* set, s32 index);                                                  // 0x801AC164
void   CharacterStartReaction(void* ch);                                                       // 0x8004E3D0
void*  ChoiceDestroy(void* c, s32 del);                                                        // 0x8020F394
void   EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);  // 0x801BA190
void   ScopedLockCtor(ScopedLock* l);                                                          // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                                               // 0x801F53B0
void*  ScreenSetFindElement(void* set, u32 hash);                                              // 0x80198F18
void*  ScreenSetFindWidget(void* set, u32 hash);                                               // 0x80198E34
void*  ScreenSetsCurrent(void* sets);                                                          // 0x8018D1E4
void   TextureRelease(void* tex);                                                              // 0x80184028
s32 TriggerDoAction(void* action, void* a1, void* a2);                                      // 0x801E0054
void*  UIWidgetFindChild(void* widget, u32 hash);                                              // 0x80199AEC
s32    WorldGetPhase(void* world);                                                             // 0x800D83A8
void   WorldSendEvent(void* world, s32 id, int a, int b, int c);                               // 0x800D87EC
u8*    fn_80021750(void);                                                                      // 0x80021750
void   fn_80041810(u8* dst, u8* src);                                                          // 0x80041810 (copy a choice)
void   fn_8001D718(u8* mgr, s32 index, u8* slot);                                              // 0x8001D718
void   fn_8004DE74(void* mon, s32 kind);                                                       // 0x8004DE74
s32    fn_8004F020(void* a, u8* choice, s32 b);                                                // 0x8004F020
s32    fn_80062C38(u8* a);                                                                     // 0x80062C38
void   fn_8013E2DC(void);                                                                      // 0x8013E2DC
void   fn_8018A948(u8* obj, f32 t);                                                            // 0x8018A948
void   fn_8018C3D0(void* p);                                                                   // 0x8018C3D0
s32    fn_8019CC7C(void* el, u32 flag);                                                        // 0x8019CC7C
s32    fn_8019CD14(void* el);                                                                  // 0x8019CD14
void*  fn_801B7198(void* anim, s32 a);                                                         // 0x801B7198
void   fn_801B72A0(void* anim, void* child);                                                   // 0x801B72A0
void   fn_801BBAC0(u8* a3, s32 a4, s32 a5, s32 a6);                                            // 0x801BBAC0
s32    fn_801ED538(u8* a3);                                                                    // 0x801ED538
void   fn_800493A8(s32 on);                                                                    // 0x800493A8

extern u8*         gBattle;               // 0x805FBBB8
extern u8          gGame[];               // 0x803B53A0
extern "C++" s32   gSelectedPart;         // 0x805F8650
extern const u32   lbl_803589D8[];        // 0x803589D8 (HUD element hashes per limb)
extern const u32   lbl_80358AD8[];        // 0x80358AD8 (HUD element hashes)
extern const u32   lbl_805F85F8[];        // 0x805F85F8 (HUD element hashes)
extern u8          lbl_80375C48[];        // 0x80375C48 (friend list hash tables at +0xE8/+0xF8/+0x108)
extern u8          lbl_803C4298[];        // 0x803C4298 (+4: object with vtable)
extern char        lbl_803F2230[];        // 0x803F2230 (name edit buffer)
extern u8          lbl_80556E80[];        // 0x80556E80 (player data block)
extern u8          lbl_805F865C;          // 0x805F865C
extern u8*         lbl_805FAF78;          // 0x805FAF78 (lightning manager)
extern u32         lbl_805FB0A0;          // 0x805FB0A0 (last disabled HUD input hash)
extern void*       lbl_805FB208[];        // 0x805FB208 (splash textures)
extern u8*         lbl_805FB248;          // 0x805FB248 (lab books section table pointer)
extern s32         lbl_805FB6B8;          // 0x805FB6B8
extern u32         lbl_805FB854;          // 0x805FB854 (32-bit frame counter)
extern u8*         lbl_805FBBE0;          // 0x805FBBE0 (save block pointer)
extern f32         lbl_805FD300;          // 0.0f
extern f32         lbl_805FD304;          // 1.0f
extern f32         lbl_805FD3DC;          // 8.0f
extern f32         lbl_805FEA50;          // 1.0f
extern f32         lbl_805FEA54;          // 0.0f
extern f32         lbl_805FEB0C;          // 0.0f
extern f32         lbl_805FEB84;          // 5.0f
}

extern "C++" void CharacterStartIdleAction(ScriptCtx* ctx);  // 0x80016204 (C++ linkage in ai_states.cpp)
extern "C++" void* gEventMgr;     // 0x805FB6C8
extern "C++" void* gScreenSets;   // 0x805FB654

#define WORLD   ((void*)(gGame + 0xD5F0))
#define PLAYER  (*(u8**)(gGame + 0xD614))

typedef u8*  (*GetObjFn)(u8*);
typedef void (*ObjFn)(u8*);
typedef void (*ObjIntFn)(u8*, s32);
typedef void (*ObjHashFn)(u8*, u32, s32, s32);

static const u32 kHudSetTag = 0x1B66AC94u;  // value at +0 of the combat HUD screen set

extern "C" {

// 0x8001CD4C
// Resets the 5 lightning slots: sets each slot's +0x198 to (+0x1A0 - 1), then re-initialises it.
void ClearMonster(void) {
    s32 off = 0;
    for (s32 i = 0; i < 5; i++) {
        u8* slot = lbl_805FAF78 + off;
        *(s32*)(slot + 0x198) = *(s32*)(slot + 0x1A0) - 1;
        fn_8001D718(lbl_805FAF78, i, slot);
        off += 0x1A4;
    }
}

// 0x800488E4
// Combat HUD: re-enables the HUD elements (limb elements + the 2 extra ones) and selects the part.
void CombatHUDEnableInput(void) {
    lbl_805FB0A0 = 0;
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    if (*(u32*)set != kHudSetTag) return;
    if (*(s32*)(gBattle + 0xA4C) == 0) {
        lbl_805F865C = 1;
        SetPartSelected(gSelectedPart);
    }
    for (s32 i = 0; i < 3; i++) {
        void* el = ScreenSetFindElement(set, lbl_80358AD8[i]);
        if (el != NULL) fn_8019CC7C(el, 1);
    }
    for (s32 i = 0; i < 2; i++) {
        void* el = ScreenSetFindElement(set, lbl_805F85F8[i]);
        if (el != NULL) fn_8019CC7C(el, 1);
    }
}

// 0x8004937C
// Re-enables all HUD elements, then restores the last disabled input.
void CombatHUDRestoreDisableState(void) {
    fn_800493A8(1);
    CombatHUDDisableInput(lbl_805FB0A0);
}

// 0x800493A8
// Sets the flag `on` on the 5 per-limb HUD elements and the 2 extra elements.
void fn_800493A8(s32 on) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    if (*(u32*)set != kHudSetTag) return;
    for (s32 i = 0; i < 5; i++) {
        void* el = ScreenSetFindElement(set, lbl_803589D8[i]);
        if (el != NULL) fn_8019CC7C(el, (u32)on);
    }
    for (s32 i = 0; i < 2; i++) {
        void* el = ScreenSetFindElement(set, lbl_805F85F8[i]);
        if (el != NULL) fn_8019CC7C(el, (u32)on);
    }
}

// 0x8004D520
// Starts an attack: runs fn_8004F020 for each of the 5 slots at +0xC50 (result 0 if any fails),
// then starts the reaction. Returns 1 if all succeeded.
s32 CharacterStartAttack(u8* ch, s32 arg) {
    u8 choice[12];
    fn_80041810(choice, *(u8**)(ch + 0xC4C) + 0x20);
    s32 ok = 1;
    for (s32 i = 0; i < 5; i++) {
        void* slot = *(void**)(ch + 0xC50 + i * 4);
        if (slot != NULL && fn_8004F020(slot, choice, arg) == 0) ok = 0;
    }
    CharacterStartReaction(ch);
    ChoiceDestroy(choice, -1);
    return ok;
}

// 0x800532A0
// Adds flags to the character; flag 2 is also mirrored into the object returned by vtable slot 0xC.
void CharacterSetFlags(u8* ch, u32 flags) {
    if (flags & 2) {
        if ((*(GetObjFn*)(*(u8**)ch + 0xC))(ch) != NULL) {
            u8* o = (*(GetObjFn*)(*(u8**)ch + 0xC))(ch);
            *(u32*)(o + 0x8) |= 2;
        }
    }
    u8* info = *(u8**)(ch + 0x4);
    *(u32*)(info + 0x20) |= flags;
}

// 0x8005A304
// Sets the ground the character stands on (handle at +0x7B0, object at +0x7B4, position/normal
// copied from the objects) and fires the "step on" triggers of the new ground.
void CharacterSetGround(u8* ch, u8* ground, u8* obj) {
    s32 prevHandle = *(s32*)(ch + 0x7B0);
    u8* prev = (u8*)fn_801B7198(&prevHandle, 0);
    s32 handle = lbl_805FB6B8;
    fn_801B72A0(&handle, ground);
    *(s32*)(ch + 0x7B0) = handle;
    *(u8**)(ch + 0x7B4) = obj;
    if (obj != NULL) {
        *(f32*)(ch + 0x7B8) = *(f32*)(obj + 0x14);
        *(f32*)(ch + 0x7BC) = *(f32*)(obj + 0x18);
        *(f32*)(ch + 0x7C0) = *(f32*)(obj + 0x1C);
    }
    if (ground != NULL) {
        u8* info = *(u8**)(ground + 0x4);
        *(f32*)(ch + 0x7C4) = *(f32*)(info + 0xC);
        *(f32*)(ch + 0x7C8) = *(f32*)(info + 0x10);
        *(f32*)(ch + 0x7CC) = *(f32*)(info + 0x14);
        *(f32*)(ch + 0x7D0) = *(f32*)(info + 0x18);
    }
    if (ground != NULL && ground != prev) {
        u8* extra = *(u8**)(ground + 0x9C);
        if (extra != NULL && ((*(u32*)(extra + 0x9C) >> 27) & 0xF)) {
            TriggerDoAction(extra + 0x9C, ch, ground);
        }
        if ((*(u32*)(ground + 0x6C) >> 27) & 0xF) {
            TriggerDoAction(ground + 0x6C, ch, ground);
        }
    }
}

// 0x8005BAD4
// mode 1: character leaves the ground (clears ground state, fires the "step off" triggers).
// mode 0: character lands (stores landing position, sends the landing event, idles if it fell far).
// Does nothing if the state (+0x78C) already equals the mode, unless `force` is set.
void CharacterResetGroundState(u8* ch, s32 mode, s32 force) {
    if (mode == 1) {
        if (*(s32*)(ch + 0x78C) == 1 && force == 0) return;
        *(u32*)(ch + 0x838) &= 0xF7FFFFFF;
        s32 prevHandle = *(s32*)(ch + 0x7B0);
        u8* prev = (u8*)fn_801B7198(&prevHandle, 0);
        if (prev != NULL) {
            if (*(s32*)(prev + 0x8C) == 0) {
                *(s32*)(ch + 0x790) = 1;
                *(u32*)(ch + 0x838) |= 0x02000000;
            }
            u8* extra = *(u8**)(prev + 0x9C);
            if (extra != NULL && ((*(u32*)(extra + 0xA4) >> 27) & 0xF)) {
                TriggerDoAction(extra + 0xA4, ch, prev);
            }
            if ((*(u32*)(prev + 0x74) >> 27) & 0xF) {
                TriggerDoAction(prev + 0x74, ch, prev);
            }
        }
        f32 zero = lbl_805FD300;
        f32 one = lbl_805FD304;
        *(s32*)(ch + 0x7B4) = 0;
        u8* info = *(u8**)(ch + 0x4);
        *(s32*)(ch + 0x7B0) = lbl_805FB6B8;
        *(f32*)(ch + 0x7B8) = zero;
        *(f32*)(ch + 0x7BC) = zero;
        *(f32*)(ch + 0x7C0) = zero;
        *(f32*)(ch + 0x7C4) = zero;
        *(f32*)(ch + 0x7C8) = zero;
        *(f32*)(ch + 0x7CC) = zero;
        *(f32*)(ch + 0x7D0) = one;
        *(f32*)(ch + 0x7A4) = zero;
        *(f32*)(ch + 0x7A8) = zero;
        *(f32*)(ch + 0x7AC) = one;
        *(s32*)(ch + 0x78C) = 1;
        *(u32*)(ch + 0x794) = lbl_805FB854;
        *(f32*)(ch + 0x798) = *(f32*)(info + 0x0);
        *(f32*)(ch + 0x79C) = *(f32*)(info + 0x4);
        *(f32*)(ch + 0x7A0) = *(f32*)(info + 0x8);
    } else if (mode == 0) {
        if (*(s32*)(ch + 0x78C) == 0 && force == 0) return;
        f32 x = *(f32*)(ch + 0x850) + *(f32*)(ch + 0x844);
        f32 y = *(f32*)(ch + 0x854) + *(f32*)(ch + 0x848);
        f32 z = *(f32*)(ch + 0x858) + *(f32*)(ch + 0x84C);
        *(f32*)(ch + 0x764) = x;
        *(u32*)(ch + 0x838) |= 0x08000000;
        *(f32*)(ch + 0x768) = y;
        *(f32*)(ch + 0x76C) = z;
        *(s32*)(ch + 0x78C) = 0;
        EventSend(gEventMgr, ch, ch, 0x8EABF5ECu, 0, 0, 1, 0);
        if (*(u32*)(*(u8**)(ch + 0x4) + 0x1C) & 0x20) {
            if ((*(u32*)(ch + 0x838) & 0x800) &&
                (*(f32*)(ch + 0x7A0) - *(f32*)(*(u8**)(ch + 0x4) + 0x8)) > lbl_805FD3DC) {
                // fell far enough: restart the idle action (first word of the object at +0xA4)
                CharacterStartIdleAction(**(ScriptCtx***)(ch + 0xA4));
                ScopedLock lock;
                ScopedLockCtor(&lock);
                (*(ObjIntFn*)(*(u8**)ch + 0x18))(ch, 2);
                ScopedLockDtor(&lock, -1);
            }
            *(u8*)(ch + 0xBDD) = 0;
        }
        u32 flags = *(u32*)(ch + 0x838);
        if (flags & 0x800) {
            *(u32*)(ch + 0x838) = flags & 0xFFFFF7FF;
            fn_801BBAC0(**(u8***)(ch + 0xA4), 0xB4843570, 0, 0);
        }
    }
}

// 0x800E4E80
// Plays clip `clip` on the animation of the object at +0x58, flags the clip (+0xC |= 0x10000),
// resets the playing state and clears the link at +0x78.
void fn_800E4E80(u8* obj, s32 clip) {
    u8* cam = *(u8**)(lbl_803C4298 + 4);
    (*(ObjFn*)(*(u8**)cam + 0x10))(cam);
    s32 handle = fn_801ED538(obj + 0x58);
    u8* anim = (u8*)fn_801B7198(&handle, 0);
    u8* c = (u8*)AnimSetGetClip(*(void**)(*(u8**)(anim + 0x9C) + 0x48), clip);
    *(u32*)(c + 0xC) |= 0x10000;
    (*(ObjIntFn*)(*(u8**)anim + 0x80))(anim, clip);
    u8* inst = (*(GetObjFn*)(*(u8**)anim + 0x10))(anim);
    fn_8018C3D0(inst);
    fn_8018A948(inst, lbl_805FEA54);
    u8* link = *(u8**)(obj + 0x78);
    if (link != NULL) {
        *(f32*)(link + 0x44) = lbl_805FEA50;
        *(u8*)(*(u8**)(obj + 0x78) + 0x54) = 0;
        *(u8**)(obj + 0x78) = NULL;
    }
    *(u8*)(obj + 0x54) = 1;
    *(f32*)(obj + 0x44) = lbl_805FEA50;
}

// 0x800EF880
// Copies the checkpoint memory-card block sent (a0->+8) into the game state.
void CheckpointMCSentCallback(u32 a0) {
    memcpy(gGame + 0xFD8, *(void**)((u8*)(uintptr_t)a0 + 8), 0xB470);
}

// 0x800EF89C
// Copies the received checkpoint memory-card block (a0->+8) into the game state.
void CheckpointMCReceivedCallback(u32 a0) {
    memcpy(gGame + 0xFD8, *(void**)((u8*)(uintptr_t)a0 + 8), 0xB470);
}

// 0x800EFF24
// Returns 1 when the value at +0xC0 of the minigame object has run out (rock crushed).
s32 CheckRockCrushed(void) {
    return *(s32*)(fn_80021750() + 0xC0) <= 0;   // cntlzw + rlwnm: zero or negative
}

// 0x800F8AA4
// Hazard touched by an entity: damages/hurts the player on contact (events + reaction kind).
void CheckHazardCollision(void* a0, void* a1) {
    u8* hz = (u8*)a0;
    u8* who = (u8*)a1;
    if (hz == NULL || who == NULL) return;
    u32 flags = *(u32*)(*(u8**)(hz + 0x4) + 0x1C);
    if ((flags & 0x8000) && who != PLAYER && *(s32*)(hz + 0x26C) == 2) {
        EventSend(gEventMgr, who, NULL, 0xDFFBED46u, 0, 0, 0, 0);
    }
    if ((*(u32*)(*(u8**)(hz + 0x4) + 0x1C) & 0x8000) && who == PLAYER && WorldGetPhase(WORLD) == 2) {
        if (*(s32*)(hz + 0x270) != 0) {
            u32 ev = 0;
            if (*(s32*)(hz + 0x268) == 1 && *(f32*)(who + 0xC88) <= lbl_805FEB0C) {
                ev = 0xEA2CD18Cu;
                *(f32*)(who + 0xC88) = lbl_805FEB84;
                fn_8004DE74(who, 5);
            }
            if (*(s32*)(hz + 0x26C) == 2) ev = 0xFBB68D5Au;
            if (ev != 0) EventSend(gEventMgr, who, NULL, ev, 0, 0, 0, 0);
        }
    } else if ((*(u32*)(*(u8**)(hz + 0x4) + 0x1C) & 0x20) && who == PLAYER && WorldGetPhase(WORLD) == 2) {
        if (*(f32*)(who + 0xC88) <= lbl_805FEB0C) {
            s32 k = fn_80062C38(hz);
            if (k == 0 || k == 1) {
                fn_8004DE74(who, 0xA);
            } else if (k == 2) {
                fn_8004DE74(who, 2);
                (*(void (**)(u8*, u32, s32, s32))(*(u8**)hz + 0xA8))(hz, 0x100C6FC7u, 0, 0);
            } else {
                fn_8004DE74(who, 2);
            }
            *(f32*)(who + 0xC88) = lbl_805FEB84;
            EventSend(gEventMgr, who, NULL, 0xEA2CD18Cu, 0, 0, 0, 0);
        }
        EventSend(gEventMgr, hz, NULL, 0x874A902Bu, 0, 0, 0, 0);
    }
}

// 0x800F8D7C
// Obstacle touched during phase 2: the player hitting it signals the obstacle; anything else
// with flag 0x10 gets the generic hit event.
void CheckObstacleCollision(void* a0, void* a1) {
    if (a0 == NULL || a1 == NULL) return;
    if (WorldGetPhase(WORLD) != 2) return;
    if (a1 == (void*)PLAYER) {
        EventSend(gEventMgr, a0, NULL, 0x4686D8CDu, 0, 0, 0, 0);
        return;
    }
    if (*(u32*)(*(u8**)((u8*)a1 + 0x4) + 0x1C) & 0x10) {
        EventSend(gEventMgr, a1, NULL, 0xDFFBED46u, 0, 0, 0, 0);
    }
}

// 0x800F973C
// Clears the save block's +0x100 answer, sends the world event `id`, and returns the answer.
u32 CheckQuestInterrupt(u32 a0) {
    *(s32*)(lbl_805FBBE0 + 0x100) = 0;
    WorldSendEvent(WORLD, (s32)a0, 0, 0, 1);
    return (u32)*(s32*)(lbl_805FBBE0 + 0x100);
}

// 0x800FA950
// Drops the reference on splash texture slot a0 (0 or 1), releasing it when the count hits 0.
void CleanupSplashScreen(BOOL a0) {
    if ((u32)a0 < 2) {
        u8* tex = (u8*)lbl_805FB208[a0];
        if (tex != NULL) {
            s32 cnt = *(s32*)(tex + 0x24) - 1;
            *(s32*)(tex + 0x24) = cnt;
            if (cnt == 0) TextureRelease(tex);
            lbl_805FB208[a0] = NULL;
        }
    }
}

// 0x8010FF1C
// Closes the currently open lab book (virtual slot 0x10 of the open section) and marks none open.
void CloseBooks(void) {
    u8* tbl = lbl_805FB248;
    if (tbl == NULL) return;
    s32 idx = *(s32*)tbl;
    if (idx == -1) return;
    u8* obj = *(u8**)(tbl + 4 + idx * 4);
    (*(ObjFn*)(*(u8**)obj + 0x10))(obj);
    *(s32*)lbl_805FB248 = -1;
}

// 0x8013C2E4
// Hides the 4 friend-list floaty entries (hide slot 2 on the name widget, plus slots 0xC/0x30
// on the other two) unless they are active and `force` != 1.
void FriendListHideFloaty(BOOL force) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* w = (u8*)ScreenSetFindWidget(set, 0x9DE0B3FEu);
    if (w == NULL) return;
    for (s32 i = 0; i < 4; i++) {
        u8* a = (u8*)UIWidgetFindChild(w, *(u32*)(lbl_80375C48 + 0xE8 + i * 4));
        u8* b = (u8*)UIWidgetFindChild(w, *(u32*)(lbl_80375C48 + 0xF8 + i * 4));
        u8* c = (u8*)UIWidgetFindChild(w, *(u32*)(lbl_80375C48 + 0x108 + i * 4));
        if (a != NULL && ((u32)force == 1 || fn_8019CD14(a) == 0)) {
            (*(ObjIntFn*)(*(u8**)(a + 0x138) + 0x30))(a, 2);
            if (b != NULL) {
                (*(ObjFn*)(*(u8**)(b + 0x138) + 0xC))(b);
                (*(ObjIntFn*)(*(u8**)(b + 0x138) + 0x30))(b, 2);
            }
            if (c != NULL) {
                (*(ObjFn*)(*(u8**)(c + 0x138) + 0xC))(c);
                (*(ObjIntFn*)(*(u8**)(c + 0x138) + 0x30))(c, 2);
            }
        }
    }
}

// 0x8013EC64
// Name entry confirm: empty name signals one flow event; otherwise the name is stored in the
// player data (+0xB40), options are saved, and another flow event is signalled.
void CheckName(void) {
    if (lbl_803F2230[0] == 0) {
        GameFlowSignal(0x68F56B4B, 0, 0);
        return;
    }
    strcpy((char*)(lbl_80556E80 + 0xB40), lbl_803F2230);
    SaveManager_SaveOptions();
    GameFlowSignal(0x72C954E3, 0, 0);
}

// 0x80140404
void CleanupMonsters(void) {
    fn_8013E2DC();
}

}  // extern "C"
