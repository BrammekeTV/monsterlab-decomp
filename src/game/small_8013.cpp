// Small functions in 0x8013xxxx (online friends list, matchup).
#include <string.h>

#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void  operator_delete(void* p);                                  // 0x801D426C
void  OSLockMutex(u8* p);                                        // 0x8023A5B0
void  OSUnlockMutex(u8* p);                                        // 0x8023A68C
void  fn_801F5714(s32 flags);                                    // 0x801F5714
void  fn_801F5734(void* arg);                                    // 0x801F5734
void  fn_801F51DC(s32 flag);                                     // 0x801F51DC
void* EventPost(s32 id, s32 flags);                              // 0x801F3918
void  fn_8032F1E8(u8* obj, void (*cb)(void), s32 a, s32 b, s32 c); // 0x8032F1E8
void  fn_801399E8(void);                                         // 0x801399E8
void  fn_8013ADD8(s32 step);                                     // 0x8013ADD8 (friends list scroll step)
s32   fn_8018CF1C(void* sets, void* widget);                     // 0x8018CF1C
void  fn_8018D648(void* sets, s32 idx);                          // 0x8018D648
s32   fn_802E64A0(u8* p, s32 n);                                 // 0x802E64A0
void  fn_80116CA8(u8* p);                                        // 0x80116CA8
u8*   fn_80116D00(u8* p, s32 del);                               // 0x80116D00
void  __register_global_object(void* obj, void (*dtor)(void*), void* handle); // 0x8032EFC8

extern "C++" void* gScreenSets;           // 0x805FB654
extern f32   gFrameTime;            // 0x805FB918
extern u8    lbl_80556AB0[];        // 0x80556AB0 (connection lock object)
extern u8    lbl_80556AC8[];        // 0x80556AC8 (lock object)
extern u8    lbl_80556E80[];        // 0x80556E80 (player data block)
extern s32   lbl_805FBB48;          // 0x805FBB48 (connection nesting counter)
extern u8    lbl_805FBBE0[];        // 0x805FBBE0 (save block pointer)
extern u8*   lbl_805FB964;          // 0x805FB964 (object with flag at +0xB1)
extern s32   lbl_805FB350;          // 0x805FB350 (scroll active)
extern s32   lbl_805FB354;          // 0x805FB354 (scroll direction)
extern f32   lbl_805FB358;          // 0x805FB358 (scroll repeat timer)
extern s32   lbl_805FB35C;          // 0x805FB35C (list index)
extern s32   lbl_805FB360;          // 0x805FB360 (list scroll offset)
extern s32   lbl_805FB368;          // 0x805FB368 (connection state, cleared on connect)
extern s32   lbl_805FB36C;          // 0x805FB36C
extern s32   lbl_805FB370;          // 0x805FB370
extern s32   lbl_805FB374;          // 0x805FB374
extern s32   lbl_805FB378;          // 0x805FB378
extern s32   lbl_805FB37C;          // 0x805FB37C
extern s32   lbl_805FB384;          // 0x805FB384
extern u32   lbl_803F0E30[];        // 0x803F0E30 (slot table)
extern u32   lbl_803F0F30[];        // 0x803F0F30 (flag table, indexed by slot table value)
extern f32   lbl_803F0CD0[3];       // 0x803F0CD0 (three float slots)
extern u8    lbl_803EA0D0[];        // 0x803EA0D0 (object passed to fn_8032F1E8)
extern f32   lbl_803F2068[3];       // 0x803F2068 (three float slots)
extern u8    lbl_803F103C[];        // 0x803F103C (global object)
extern u8    lbl_803F1030[];        // 0x803F1030 (global object handle)
extern f32   lbl_805FEF68;          // 0x805FEF68 (float constant)
extern f32   lbl_805FEF6C;          // 0x805FEF6C (float constant)
extern f32   lbl_805FEF90;          // 0x805FEF90 (float constant, zero)
extern f32   lbl_805FEF94;          // 0x805FEF94 (float constant, scroll timer reset)
extern f32   lbl_805FEF98;          // 0x805FEF98 (float constant, scroll timer start)
extern f32   lbl_805FEF9C;          // 0x805FEF9C (float constant)
}

// 0x80132BD8
// Clears the three screen-set widgets listed at obj+0x14 (each entry's widget at +0x100),
// then resets the three screen sets.
extern "C" void fn_80132BD8(u8* p) {
    for (s32 i = 0; i < 3; i++) {
        u8* slot = *(u8**)(p + 0x14 + i * 4);
        if (slot != 0) {
            fn_8018CF1C(gScreenSets, *(void**)(slot + 0x100));
        }
    }
    for (s32 i = 0; i < 3; i++) {
        fn_8018D648(gScreenSets, i);
    }
}

// 0x801330E4
// Looks through the three pointer slots at p+0x14 for one whose id (+0x18) matches the key's
// id (+0x18). A match is cleared and the address of the slot's index (p + 4*k) is returned;
// otherwise p is returned.
u8* fn_801330E4(u8* p, u8* key) {
    s32 want = *(s32*)(key + 0x18);
    for (s32 k = 0; k < 3; k++) {
        u8* elem = *(u8**)(p + 0x14 + k * 4);
        if (elem != 0 && *(s32*)(elem + 0x18) == want) {
            *(void**)(p + 0x14 + k * 4) = 0;
            return p + k * 4;
        }
    }
    return p;
}

// 0x80135300
// Deleting-destructor tail: frees p when del > 0 (no base destructor); returns p.
u8* fn_80135300(u8* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80135340
// Deleting-destructor tail: frees p when del > 0 (no base destructor); returns p.
u8* fn_80135340(u8* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x80135380
// Deleting-destructor tail: frees p when del > 0 (no base destructor); returns p.
u8* fn_80135380(u8* p, s32 del) {
    if (p != 0 && del > 0) operator_delete(p);
    return p;
}

// 0x801399AC
// Sets up the three-float state at lbl_803F0CD0 (values A, B, A) and starts the
// fn_8032F1E8 animation on lbl_803EA0D0 with fn_801399E8 as its callback.
void fn_801399AC(void) {
    lbl_803F0CD0[0] = lbl_805FEF6C;
    lbl_803F0CD0[1] = lbl_805FEF68;
    lbl_803F0CD0[2] = lbl_805FEF6C;
    fn_8032F1E8(lbl_803EA0D0, fn_801399E8, 0, 0x48, 0x180);
}

// 0x8013A03C
// Under the lock at lbl_80556AC8: sends a flow signal, and sets the flag at +0xB1 of the
// object at lbl_805FB964 to 1 when that flag is non-zero.
void fn_8013A03C(void) {
    OSLockMutex(lbl_80556AC8);
    GameFlowSignal((s32)0xE2C48A06, 0, 0);
    u8* obj = lbl_805FB964;
    if (obj != 0 && obj[0xB1] != 0) {
        obj[0xB1] = 1;
    }
    OSUnlockMutex(lbl_80556AC8);
}

// 0x8013B434
// While the friends list is held (lbl_805FB350), counts the repeat timer down by frame
// time. When the button is released the hold ends; when the timer runs out, it steps the
// list again (fn_8013ADD8 with lbl_805FB354) and restarts the timer.
void FriendListUpdate(void) {
    if (lbl_805FB350 == 0) return;
    if (!IsButtonDown(1)) {
        lbl_805FB350 = 0;
        return;
    }
    f32 t = lbl_805FB358 - gFrameTime;
    lbl_805FB358 = t;
    if (t <= lbl_805FEF90) {
        fn_8013ADD8(lbl_805FB354);
        lbl_805FB358 = lbl_805FEF94;
    }
}

// 0x8013B990
// Clears the connection state words, resets the link layer (fn_801F5734, fn_801F51DC,
// fn_801F5714) and starts the network.
void ServerConnect(void) {
    lbl_805FB368 = 0;
    lbl_805FB36C = 0;
    lbl_805FB370 = 0;
    lbl_805FB374 = 0;
    lbl_805FB378 = 0;
    lbl_805FB37C = 0;
    lbl_805FB384 = 0;
    fn_801F5734(0);
    fn_801F51DC(0);
    fn_801F5714(-1);
    InitNetwork();
}

// 0x8013BC2C
// Sends the friend-list decline event (0x37) after raising the link flag 1.
void FriendsListSendDecline(void) {
    fn_801F5714(1);
    EventPost(0x37, 0);
}

// 0x8013BC68
// Looks up the current list entry's status word (through the two tables) and returns its
// online bit (bit 1).
BOOL FriendsListIsPlayerOnline(void) {
    s32 slot = lbl_805FB35C + lbl_805FB360;
    u32 v = lbl_803F0E30[slot];
    u32 w = lbl_803F0F30[v];
    return (BOOL)((w >> 1) & 1);
}

// 0x8013BC9C
// Same lookup as FriendsListIsPlayerOnline; returns 1 when the entry's bit 2 or bit 3 is set
// (busy), otherwise 0.
BOOL FriendsListIsPlayerBusy(void) {
    s32 slot = lbl_805FB35C + lbl_805FB360;
    u32 v = lbl_803F0E30[slot];
    u32 w = lbl_803F0F30[v];
    if ((w & 0x4) != 0 || (w & 0x8) != 0) return 1;
    return 0;
}

// 0x8013BE18
// Fills the local-player record from EventPost(0x3E): two words from the save block (0xA28,
// 0xA2C) and the name string at lbl_80556E80+0xB40.
void TransmitMe(void) {
    u8* p = (u8*)EventPost(0x3E, 0);
    if (p == 0) return;
    u8* save = *(u8**)lbl_805FBBE0;
    *(s32*)(p + 0x4) = *(s32*)(save + 0xA28);
    *(s32*)(p + 0x8) = *(s32*)(save + 0xA2C);
    strcpy((char*)(p + 0xC), (const char*)(lbl_80556E80 + 0xB40));
}

// 0x8013BE7C
// Sends the aggressor-declined event (0x3C) after raising the link flag 1.
void AggressorDeclined(void) {
    fn_801F5714(1);
    EventPost(0x3C, 0);
}

// 0x8013BEAC
// Under the lock at lbl_80556AB0 (with the nesting counter bumped), reports whether the list
// at lbl_80556E80+0x40 is full: fn_802E64A0 returns its count, full when that is 0x40.
u32 IsListFull(void) {
    OSLockMutex(lbl_80556AB0);
    s32 count = lbl_805FBB48;
    lbl_805FBB48 = count + 1;
    s32 n = fn_802E64A0(lbl_80556E80 + 0x40, 0x40);
    u32 full = (n == 0x40) ? 1 : 0;
    OSUnlockMutex(lbl_80556AB0);
    lbl_805FBB48 = lbl_805FBB48 - 1;
    return full;
}

// 0x8013BF20
// Starts a continuous scroll of the friends list upward (step +1).
void FriendListScrollUp(void) {
    fn_8013ADD8(1);
    lbl_805FB350 = 1;
    lbl_805FB354 = 1;
    lbl_805FB358 = lbl_805FEF98;
}

// 0x8013BF58
// Starts a continuous scroll of the friends list downward (step -1).
void FriendListScrollDown(void) {
    fn_8013ADD8(-1);
    lbl_805FB350 = 1;
    lbl_805FB354 = -1;
    lbl_805FB358 = lbl_805FEF98;
}

// 0x8013C16C
// Sends a flow signal for the challenge: one id when the save's flag at +0x10E8 is 0,
// another when it is set.
void FriendListChallenge(void) {
    u8* save = *(u8**)lbl_805FBBE0;
    if (*(s32*)(save + 0x10E8) == 0) {
        GameFlowSignal((s32)0x6D46D31D, 0, 0);
        return;
    }
    GameFlowSignal((s32)0x21AB8636, 0, 0);
}

// 0x8013C59C
// Static initializer for the global object at lbl_803F103C: sets the three-float state, runs
// its constructor fn_80116CA8, and registers its destructor fn_80116D00.
void fn_8013C59C(void) {
    lbl_803F2068[0] = lbl_805FEF90;
    lbl_803F2068[1] = lbl_805FEF9C;
    lbl_803F2068[2] = lbl_805FEF90;
    fn_80116CA8(lbl_803F103C);
    __register_global_object(lbl_803F103C, (void (*)(void*))fn_80116D00, lbl_803F1030);
}

// 0x8013CF6C
// Hides the two matchup panels (UI hashes 0x8FB60C54 and 0x6530D136).
void MatchupDeInit(void) {
    UIElement_SetVisibleFlag(0x8FB60C54, 0);
    UIElement_SetVisibleFlag(0x6530D136, 0);
}
