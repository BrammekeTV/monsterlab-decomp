// Junction / exploration setup functions in 0x800Fxxxx.
#include "types.h"
#include "game/part.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void* CharacterFind(s32 id);                              // 0x800E876C
void  ExplorationPromptClear(BOOL animate);               // 0x80016EA0
u8*   LabAllocMonster(u8* inv);
void  MonsterBuildPreset(u8* m, s32 preset);
u8*   ObjDbLookup(void* db, s32 id);
u8*   ObjRefDtor(u8* ref, s32 flags);
void  PartSetHealth(PartInstance* p, s32 hp);
void  PartSetQuality(PartInstance* p, s32 q);
s32   PathGetNodeId(u8* p);                               // 0x800D56F8
s32   PathGetNodeType(u8* p);
BOOL  PathIsAtJunction(u8* p);                            // 0x800D5924
u8*   SaveGetCurrentMonster(u8* s);
void  ScopedLockCtor(u8* l);
void  ScopedLockDtor(u8* l, s32 flags);
void  SessionReenable(u8* session, s32 id);
s32   WorldGetPhase(void* world);                         // 0x800D83A8
void  fn_80016FD0(void* c, int animate);
void  fn_80044F20(void);
s32  fn_8004D3F8(void* c, const char* name, s32 a, s32 b);
void  fn_8004D760(u8* c, u8* monster);
void* fn_8004DAF8(void* c, s32 a, s32 b);
void fn_80053418(u8* arg0);
void  fn_800D3428(void);
s32   fn_800D3930(void* db);
u8*   fn_800D393C(void* db, s32 i);
u8*   fn_800D5838(u8* ref, s32 text);
void* fn_800D59A4(u8* ref, s32 i);
void  fn_800D59F4(void* pathOrRef, s32 state);
void fn_800D5C48(u8* arg0, s32 arg1);
s32 fn_800D68CC(u8* arg0, s32 arg1, s32 arg2);
u8*  fn_800D6B44(void);
extern u8   gGame[];                                      // 0x803B53A0
extern void* gObjDb;                                      // 0x805FB180
extern u8*  gSaveSystem;                                  // 0x805FBBE0
extern const char lbl_805F8DA8[];                         // "idlereg"
}

#define WORLD   (gGame + 0xD5F0)
#define PLAYER  (*(u8**)(gGame + 0xD614))

// Object reference record (0x404 bytes) copied onto the stack from the object database.
struct ObjRefBuf { s32 w[0x101]; };

typedef s32 (*VFn0)(void* self);
typedef s32 (*VFn1)(void* self, s32 a);

extern "C" {

// 0x800F0358
// Locks a junction's path (via its object reference) while exploring.
void LockJunctionPath(u32 a0, BOOL a1, u32 a2) {
    (void)a2;
    if (WorldGetPhase(WORLD) == 2) {
        ObjRefBuf ref = *(ObjRefBuf*)ObjDbLookup(gObjDb, (s32)a0);
        fn_800D68CC((u8*)&ref, a1, 1);
        ObjRefDtor((u8*)&ref, -1);
    }
}

// 0x800F040C
// Unlocks a junction's path (via its object reference) while exploring.
void UnlockJunctionPath(u32 a0, BOOL a1) {
    if (WorldGetPhase(WORLD) == 2) {
        ObjRefBuf ref = *(ObjRefBuf*)ObjDbLookup(gObjDb, (s32)a0);
        fn_800D68CC((u8*)&ref, a1, 0);
        ObjRefDtor((u8*)&ref, -1);
    }
}

// 0x800F058C
// For each of the junction's 3 occupants: arg1 set -> clear the 0x80 flag (enable via vtable +0x14
// under a scoped lock if flag 0x200000 is set, else set 0x800); arg1 clear -> disable via vtable +0x18
// under a scoped lock, then set 0x80.
void fn_800F058C(s32 arg0, s32 arg1) {
    if (gObjDb != NULL) {
        ObjRefBuf ref = *(ObjRefBuf*)ObjDbLookup(gObjDb, arg0);
        for (u8 i = 0; i < 3; i++) {
            s32 id = (s32)(intptr_t)fn_800D59A4((u8*)&ref, i);
            if (id == 0) continue;
            u8* c = (u8*)CharacterFind(id);
            if (arg1 != 0) {
                if (c != NULL) {
                    u8* def = *(u8**)(c + 4);
                    u32 flags = *(u32*)(def + 0x20);
                    if (flags & 0x200000) {
                        u8 lock[4];
                        ScopedLockCtor(lock);
                        (*(VFn1*)(*(u8**)c + 0x14))(c, 1);
                        ScopedLockDtor(lock, -1);
                    } else {
                        *(u32*)(def + 0x20) = flags | 0x800;
                    }
                    def = *(u8**)(c + 4);
                    *(u32*)(def + 0x20) &= 0xFFFFFF7F;
                }
            } else {
                if (c != NULL) {
                    u8 lock[4];
                    ScopedLockCtor(lock);
                    (*(VFn1*)(*(u8**)c + 0x18))(c, 0);
                    ScopedLockDtor(lock, -1);
                    u8* def = *(u8**)(c + 4);
                    *(u32*)(def + 0x20) |= 0x80;
                }
            }
        }
        ObjRefDtor((u8*)&ref, -1);
    }
}

// 0x800F070C
// Sets the state of the junction the player's character is currently on.
void SetCurrentJunctionState(u32 /*0x06*/ a0) {
    s32 state = a0 != 0;
    u8* chr = PLAYER;
    if ((*(u32*)(*(u8**)(chr + 4) + 0x1C) & 0x10) && *(u8**)(chr + 0xA00) != NULL) {
        fn_800D59F4(*(u8**)(chr + 0xA00), state);
        if (a0 == 0 && PathGetNodeType(*(u8**)(chr + 0xA00)) == 2) {
            s32 id = PathGetNodeId(*(u8**)(chr + 0xA00));
            if (id != 0) SessionReenable(gGame + 0x5C8, id);
        }
        fn_800F058C(PathGetNodeId(*(u8**)(chr + 0xA00)), state);
    }
}

// 0x800F07D0
// Sets a junction's state; refreshes the player's prompt if the player stands on it.
void SetJunctionState(u32 a0, u32 /*0x06*/ a1) {
    s32 state = a1 != 0;
    if (gObjDb != NULL) {
        ObjRefBuf ref = *(ObjRefBuf*)ObjDbLookup(gObjDb, (s32)a0);
        if (a1 == 0 && PathGetNodeType((u8*)&ref) == 2 && a0 != 0) {
            SessionReenable(gGame + 0x5C8, (s32)a0);
        }
        fn_800D59F4(&ref, state);
        u8* chr = PLAYER;
        if ((*(u32*)(*(u8**)(chr + 4) + 0x1C) & 0x10) && chr != NULL) {
            u8* path = *(u8**)(chr + 0xA00);
            if (path != NULL && a0 == (u32)PathGetNodeId(path)) {
                if (a1 != 0) fn_80016FD0(chr, 0);
                else ExplorationPromptClear(0);
            }
        }
        fn_800F058C((s32)a0, (s32)a1);
        ObjRefDtor((u8*)&ref, -1);
    }
}

// 0x800F0924
// Sets a junction's text; refreshes the player's prompt if the player stands on it.
void SetJunctionText(u32 a0, u32 /*0x11*/ a1) {
    if (WorldGetPhase(WORLD) == 2) {
        ObjRefBuf ref = *(ObjRefBuf*)ObjDbLookup(gObjDb, (s32)a0);
        fn_800D5838((u8*)&ref, (s32)a1);
        u8* chr = PLAYER;
        if (chr != NULL && a0 == (u32)PathGetNodeId(*(u8**)(chr + 0xA00))) {
            fn_80016FD0(chr, 0);
        }
        ObjRefDtor((u8*)&ref, -1);
    }
}

// 0x800F0A08
// Clears a junction's text; clears the prompt if the player stands on it.
void ClearJunctionText(u32 a0) {
    if (WorldGetPhase(WORLD) == 2) {
        ObjRefBuf ref = *(ObjRefBuf*)ObjDbLookup(gObjDb, (s32)a0);
        fn_800D5838((u8*)&ref, 0);
        u8* chr = PLAYER;
        if (chr != NULL && a0 == (u32)PathGetNodeId(*(u8**)(chr + 0xA00))) {
            ExplorationPromptClear(0);
        }
        ObjRefDtor((u8*)&ref, -1);
    }
}

// 0x800F0C7C
// Maps the script's exploration area value to a save-block slot and queues it at save + 0x104.
void ExplorationPreInit(u32 /*0x4A*/ a0) {
    s32 v = -1;
    fn_800D3428();
    fn_800D6B44();
    switch ((s32)a0) {
    case 0: v = 0; break;
    case 1: v = 1; break;
    case 2: v = 2; break;
    case 3: v = 3; break;
    case 4: v = 5; break;
    case 5: v = 6; break;
    }
    if (gSaveSystem != NULL) *(s32*)(gSaveSystem + 0x104) = v;
}

// 0x800F0D20
// Exploration start: commits the queued area, enables every junction at a path junction, and when
// the player is an animated character prepares the monster party (full health) and idle animation.
void ExplorationInit(void) {
    fn_80044F20();
    if (gSaveSystem != NULL) {
        s32 queued = *(s32*)(gSaveSystem + 0x104);
        if (queued != -1) {
            s32 old = *(s32*)(gSaveSystem + 0x108);
            *(s32*)(gSaveSystem + 0x10C) = old;
            *(s32*)(gSaveSystem + 0x108) = queued;
            *(s32*)(gSaveSystem + 0x104) = -1;
            if (old == -1) *(s32*)(gSaveSystem + 0x10C) = *(s32*)(gSaveSystem + 0x108);
        }
    }
    if (gObjDb != NULL) {
        for (u32 i = 0; i < (u32)fn_800D3930(gObjDb); i++) {
            ObjRefBuf ref = *(ObjRefBuf*)fn_800D393C(gObjDb, (s32)i);
            if (PathIsAtJunction((u8*)&ref) != 0) fn_800D5C48((u8*)&ref, 1);
            ObjRefDtor((u8*)&ref, -1);
        }
    }
    if (*(s32*)(gGame + 0xFE0) == 0) {
        u8* chr = PLAYER;
        if (chr != NULL && (*(u32*)(*(u8**)(chr + 4) + 0x1C) & 0x2000)) {
            if (*(s32*)(gSaveSystem + 0x10E8) == 0) {
                u8* m = LabAllocMonster(gSaveSystem + 0xA30);
                if (m != NULL) {
                    MonsterBuildPreset(m, 0);
                    *(u8*)(m + 0x4A) = 1;
                    for (s32 i = 0; i < 5; i++) {
                        PartInstance* p = *(PartInstance**)(m + 4 + i * 4);
                        if (p != NULL) {
                            PartSetQuality(p, 100);
                            PartSetHealth(p, PartGetMaxHealth(p));
                        }
                    }
                }
            }
            if ((*(VFn0*)(*(u8**)chr + 0xF0))(chr) == 0) {
                u8* m = SaveGetCurrentMonster(gSaveSystem);
                for (s32 i = 0; i < 5; i++) {
                    PartInstance* p = *(PartInstance**)(m + 4 + i * 4);
                    if (p != NULL) PartSetHealth(p, PartGetMaxHealth(p));
                }
                fn_8004D760(chr, m);
                fn_8004DAF8(chr, 0, 0);
            }
            fn_80053418(chr);
            fn_8004D3F8(chr, lbl_805F8DA8, 1, 1);
        }
    }
}

}  // extern "C"
