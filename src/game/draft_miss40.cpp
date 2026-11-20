// Misc game functions from the miss40 batch: battle/match request helpers, script-callable
// setters for NIS and tutorial state, entity facing/transform callbacks, and the save banner.
#include "types.h"
#include "engine/math.h"
#include "game/part.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
// Callees (prototypes from their defining files or from the asm)
void*  CharacterFind(s32 id);                                   // 0x800E876C
void   CharacterFaceDirection(Character* c, const Vec3* dir, BOOL now);  // 0x8005B650
void*  ChoiceDestroy(void* c, s32 del);                         // 0x8020F394
void   ChoiceInit(u8* c);
void*  EventPost(s32 id, void* sender);                         // 0x801F3918
int    SysSprintf(char* buf, const char* fmt, ...);
void   fn_801F5714(s32 flags);                                  // 0x801F5714
void   fn_8013A780(s32 a, s32 b);                                       // matchup callback
s32    fn_802E9AB0(u8* p, s32 a, s32 b, s32 c, void* cb, s32 d, s32 e, s32 f);
s32    fn_802E6528(s32* args, s32 a, s32 b, s32 c, s32 d);      // 0x802E6528, signature guessed by draft_batch
void   fn_8023A5B0(u8* p);                                      // 0x8023A5B0
void   fn_8023A68C(u8* p);                                      // 0x8023A68C
f32    fn_800FB5B8(f32 a, f32 b);                               // 0x800FB5B8
s32    fn_8015BA30(void* p);                                    // 0x8015BA30
void   fn_8014DE68(void);                                       // 0x8014DE68
void   fn_80155D70(void);
void   fn_80154974(void);
void   fn_801566E8(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
void   fn_801D66C4(f32 a, f32 b);                               // 0x801D66C4
void*  fn_802209CC(void* dst, s32 val, u32 n);
s32    fn_801132BC(s32 v);
s32    fn_801138F8(void);
s32    fn_8020703C(void);
s32    fn_80267B14(u32* buf);
s32    fn_802665B0(u8* a, char* b);

// Globals (declared with the type each file loads them as)
extern u8*  gBattle;
extern s32  gScreenWidth;
extern s32  gScreenHeight;
extern u8   sRendDLs[];
extern s32  lbl_805F90A0;
extern u8   lbl_805F902C;
extern s32  lbl_805F9030;
extern const char lbl_805F990C[];   // format string "%s/%s"
extern const char lbl_805F9914[];   // second path component
extern u8   lbl_80386CA0[];
extern u8   lbl_80556AB0[];
extern u8   lbl_80556E80[];
extern u8*  lbl_805FAF78;
extern u8   lbl_805FBBE0[];         // holds the save block pointer
extern s32  lbl_805FB378;
extern s32  lbl_805FBB48;
extern s32  lbl_805FB3B8;
extern u8   lbl_805FB3B3;
extern s32  lbl_805FB4BC;
extern s32  lbl_805FB4C8;
extern u8   lbl_805FB65C;
extern u8*  lbl_805FB75C;
extern f32  lbl_805F9390;
extern f32  lbl_805F9394;
extern f32  lbl_805FD1E8;
extern f32  lbl_805FEB0C;
}

extern "C" {

// 0x8019600C
// Makes sure the current display-list record has room, then sets up the frame's viewport and
// ortho projection from the screen size.
s32 fn_8019600C(void* gx) {
    if ((s32)(fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94) + 0x13E8) > lbl_805FB4C8) {
        fn_8014DE68();
        fn_8015BA30(sRendDLs + lbl_805FB4BC * 0x94);
    }
    fn_801D66C4(lbl_805F9390, lbl_805F9394);
    *(s32*)(lbl_805FB75C + 0x188) = lbl_805FB65C;
    fn_80155D70();
    fn_80154974();
    fn_801566E8(0.0f, (f32)gScreenWidth, (f32)gScreenHeight, 0.0f, -lbl_805F9390, -lbl_805F9394);
    return 0;
}

// 0x8022ED60
// Zeroes the 0x988-byte block at p and returns p.
u8* fn_8022ED60(u8* p) {
    fn_802209CC(p, 0, 0x988);
    return p;
}

// 0x8013BB68
// Starts a matchup request under the connection lock: when the request is accepted, copies the
// two player-data words for the matchup slot and sends them with fn_802E6528.
void SendMatchupRequest(void) {
    s32 args[2];
    s32 res;
    u8* slot;

    MatchupCleanup();
    fn_8023A5B0(lbl_80556AB0);
    lbl_805FBB48 += 1;
    res = fn_802E9AB0(&lbl_805F902C, 1, 2, 0, (void*)fn_8013A780, 0, 0, 0);
    if (res != 0) {
        slot = lbl_80556E80 + (lbl_805F902C << 5);
        lbl_805FB378 = 1;
        args[1] = *(s32*)(slot + 0x354);
        lbl_805F9030 = (s32)lbl_805F902C;
        args[0] = *(s32*)(slot + 0x350);
        fn_802E6528(args, 8, args[1], lbl_805F902C, 1);
    }
    fn_8023A68C(lbl_80556AB0);
    lbl_805FBB48 -= 1;
}

// 0x8013DD1C
// Posts event 0x40 and clears the two monster-change flags.
void SendMonsterChangePacket(void) {
    EventPost(0x40, NULL);
    lbl_805FB3B8 = 0;
    lbl_805FB3B3 = 0;
}

// 0x8013FDBC
// Sets the flag from fn_801F5714 and posts event 0x46 (rematch declined).
void SendRematchDeclined(void) {
    fn_801F5714(1);
    EventPost(0x46, NULL);
}

// 0x8013FDF8
// Sets the flag from fn_801F5714 and posts event 0x4A (time is up).
void SendTimeIsUp(void) {
    fn_801F5714(1);
    EventPost(0x4A, NULL);
}

// 0x800F927C
// Sets up the challenge projector on the character with id 0x1A01DC01 for the NIS.
void SetChallengeProjectorsForNIS(void) {
    SetupChallengeProjector(CharacterFind(0x1A01DC01), 0U, 0U);
}

// 0x8013EA98
// Switches the current MP monster to lbl_805F90A0 when it differs from fn_801138F8().
void SetCurrentMPMonster(void) {
    s32 idx = lbl_805F90A0;
    if (idx != fn_801138F8()) {
        fn_801132BC(idx);
    }
}

// 0x8001CDB0
// Copies the monster's +0xA30 block entry (indexed by the current monster slot) into the save
// block's +0x114 field, when a save block exists and has a non-zero count at +0x10E8.
void SetCurrentMonster(void) {
    u8* save = *(u8**)lbl_805FBBE0;
    if (lbl_805FAF78 != NULL && *(s32*)(save + 0x10E8) > 0) {
        s32 idx = *(s32*)(lbl_805FAF78 + 0xAD8);
        *(s32*)(save + 0x114) = *(s32*)(save + (idx << 2) + 0xA30);
    }
}

// 0x800489D4
// Starts the enemy tutorial action: builds a choice record (0x124, 0, 3), stores it in the battle
// tutorial block, sets the enemy action timer from the game data, then destroys the choice.
void SetEnemyTutorialAction(void) {
    u8 choice[0xC];
    u8* tut;
    u8* gdata;
    u8* row;

    ChoiceInit(choice);
    *(s32*)(choice + 0x0) = 0x124;
    *(u8*)(choice + 0x4)  = 0;
    *(s32*)(choice + 0x8) = 3;

    tut = *(u8**)(gBattle + 0xA04);
    *(s32*)(tut + 0x38) = 0x124;
    *(u8*)(tut + 0x3C)  = *(u8*)(choice + 0x4);
    *(s32*)(tut + 0x40) = *(s32*)(choice + 0x8);

    gdata = gGameDb->data;
    row = *(u8**)(gdata + 0x99D0);
    *(f32*)(gBattle + 0xAE0) = *(f32*)(row + 0xDC) - lbl_805FD1E8;
    *(s32*)(gBattle + 0xAE8) = 1;
    *(s32*)(gBattle + 0xAD8) = 1;
    ChoiceDestroy(choice, -1);
}

// 0x800FB66C
// Faces an entity toward (a1, a2) when its flag word (entity+0x4, +0x1C) has bit 0x10 set,
// then stores the turn angle computed by fn_800FB5B8 at +0xDC.
void SetEntityFacing(void* a0, f32 a1, f32 a2, u32 a3) {
    u8* ent = (u8*)a0;
    if (*(s32*)(*(u8**)(ent + 0x4) + 0x1C) & 0x10) {
        Vec3 dir;
        dir.x = a1;
        dir.y = a2;
        dir.z = lbl_805FEB0C;
        CharacterFaceDirection((Character*)a0, &dir, a3 != 0);
        *(f32*)(ent + 0xDC) = fn_800FB5B8(a2, a1);
    }
}

// 0x801B8658
// Copies the four orientation floats from the script argument block into the entity's
// transform (+0xC..+0x18) and sets the entity's update flags (+0x20).
void SetEntityOrientationCallback(u32 a0, u32 a1, u32 a2) {
    u8* src = (u8*)(uintptr_t)a0;
    u8* ent = *(u8**)((u8*)(uintptr_t)a1 + 0x4);
    *(f32*)(ent + 0xC)  = *(f32*)(src + 0x0);
    *(f32*)(ent + 0x10) = *(f32*)(src + 0x4);
    *(f32*)(ent + 0x14) = *(f32*)(src + 0x8);
    *(f32*)(ent + 0x18) = *(f32*)(src + 0xC);
    *(u32*)(ent + 0x20) |= 0x1000C000u;
}

// 0x801B85B8
// Copies the three position floats from the script argument block into the entity's position
// (+0x0..+0x8) and sets the entity's update flags (+0x20).
void SetEntityPositionCallback(u32 a0, u32 a1, u32 a2) {
    u8* src = (u8*)(uintptr_t)a0;
    u8* ent = *(u8**)((u8*)(uintptr_t)a1 + 0x4);
    *(f32*)(ent + 0x0) = *(f32*)(src + 0x0);
    *(f32*)(ent + 0x4) = *(f32*)(src + 0x4);
    *(f32*)(ent + 0x8) = *(f32*)(src + 0x8);
    *(u32*)(ent + 0x20) |= 0x1000C000u;
}

// 0x80206AA4
// Builds the save-banner path from the save name (fn_80267B14) and checks it with fn_802665B0.
// Returns 1 when the path lookup finds nothing, 0 otherwise; returns 0 early if fn_8020703C fails.
// The original never reads the title/sub/out arguments.
s32 SaveSetBanner(const char* title, const char* sub, s32* out) {
    u32  name[16];
    char path[0x40];
    s32  i;

    if (fn_8020703C() == 0) {
        return 0;
    }
    for (i = 0; i < 16; i++) {
        name[i] = 0;
    }
    fn_80267B14(name);
    SysSprintf(path, lbl_805F990C, (char*)name, lbl_805F9914);
    return fn_802665B0(lbl_80386CA0, path) == 0;
}

}  // extern "C"
