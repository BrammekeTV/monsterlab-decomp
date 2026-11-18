// Small functions in 0x8009xxxx (robot evolution experiment).
#include "types.h"

extern "C" {
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void  GameFlowSignal(s32 id, s32 a, s32 b);                       // 0x8000C7F4
extern void* gEventMgr;                                           // 0x805FB6C8
void  operator_delete(void* p);                                   // 0x801D426C
u8*   fn_8006FBDC(void);                                          // 0x8006FBDC
s32   fn_8006FBFC(u8* obj);                                       // 0x8006FBFC
void  fn_8006FA44(u8* a3, s32 a4, s32 a5, s32 a6, s32 a7, s32 a8); // 0x8006FA44
void  fn_800923DC(u8* p);                                         // 0x800923DC
void  fn_800927B0(u8* x);                                         // 0x800927B0
u8*   fn_80092CC4(u8* obj);                                       // 0x80092CC4
extern f32 lbl_805FDA90;                                          // 0x805FDA90 (float)
extern f32 lbl_805FDAA0;                                          // 0x805FDAA0 (float)
extern f32 lbl_805FDAA8;                                          // 0x805FDAA8 (float)
extern f32 lbl_805FDB00;                                          // 0x805FDB00 (float)
extern f32 lbl_805FDB08;                                          // 0x805FDB08 (float)
extern f32 lbl_803C6A4C[3];                                       // 0x803C6A4C
extern u8  lbl_803C6A38[];                                        // 0x803C6A38
extern u8* lbl_803C6A58[3];                                       // 0x803C6A58 (door table)

// 0x80090210
// Adds dt to the float at +0x8C; when it reaches lbl_805FDAA0, signal the game flow.
void fn_80090210(u8* p, f32 dt) {
    f32 t = *(f32*)(p + 0x8C) + dt;
    *(f32*)(p + 0x8C) = t;
    if (!(t >= lbl_805FDAA0)) return;
    GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x80090304
// Same as fn_80090210 with threshold lbl_805FDAA8.
void fn_80090304(u8* p, f32 dt) {
    f32 t = *(f32*)(p + 0x8C) + dt;
    *(f32*)(p + 0x8C) = t;
    if (!(t >= lbl_805FDAA8)) return;
    GameFlowSignal((s32)0x88C6E941u, 0, 0);
}

// 0x80091BF0
// Intro smasher done: counts +0x810 on the robot object when its state is 4.
void RobotEvolutionExperiment_IntroSmasherDone(void) {
    u8* o = fn_8006FBDC();
    if (o == NULL) return;
    if (fn_8006FBFC(o) != 4) return;
    *(s32*)(o + 0x810) += 1;
}

// 0x80091C3C
// Intro robot countdown: passes the object at +0x10 to fn_80092CC4, then fn_800927B0 if that returns nonzero.
void RobotEvolutionExperiment_IntroRobotShowCountdown(u8* p) {
    u8* x = fn_80092CC4(*(u8**)(p + 0x10));
    if (x != 0) fn_800927B0(x);
}

// 0x80091C6C
// Intro robot done: counts +0x80C on the robot object when its state is 4.
void RobotEvolutionExperiment_IntroRobotDone(void) {
    u8* o = fn_8006FBDC();
    if (o == NULL) return;
    if (fn_8006FBFC(o) != 4) return;
    *(s32*)(o + 0x80C) += 1;
}

// 0x80091CB8
// Sets up the 3-float table at lbl_803C6A4C, then calls fn_8006FA44 on the block at lbl_803C6A38.
void fn_80091CB8(void) {
    lbl_803C6A4C[0] = lbl_805FDA90;
    lbl_803C6A4C[1] = lbl_805FDAA0;
    lbl_803C6A4C[2] = lbl_805FDA90;
    fn_8006FA44(lbl_803C6A38, (s32)0xDE9DD5C2u, 3, (s32)0x7A150909u, (s32)0x63103698u, (s32)0x74B93DC2u);
}

// 0x80091E44
// Resets a door-record: unregisters its table slot (+0x8) and clears state/timer.
void fn_80091E44(u8* p) {
    s32 idx = *(s32*)(p + 0x8);
    if (idx != -1) lbl_803C6A58[idx] = NULL;
    *(s32*)(p + 0x0) = 0;
    *(s32*)(p + 0x4) = -1;
    *(s32*)(p + 0x8) = -1;
    *(u8*)(p + 0xC) = 0;
    *(f32*)(p + 0x10) = lbl_805FDB00;
}

// 0x80091E88
// Counts down the timer at +0x10 while active (+0xC); on expiry, disarms and sends event 0xCF9DDB47.
void fn_80091E88(u8* p, f32 dt) {
    if (*(u8*)(p + 0xC) == 0) return;
    f32 t = *(f32*)(p + 0x10) - dt;
    *(f32*)(p + 0x10) = t;
    if (!(t <= lbl_805FDB00)) return;
    *(u8*)(p + 0xC) = 0;
    void* target = *(void**)p;
    *(s32*)(p + 0x4) = 0;
    EventSend(gEventMgr, target, NULL, 0xCF9DDB47u, 0, 0, 1, 0);
}

// 0x80091F28
// Sends event 0xCF9DDB47 for the object at p. The state==2 early-out in the original asm is unreachable.
void fn_80091F28(u8* p, s32 /*unused*/) {
    EventSend(gEventMgr, *(void**)p, NULL, 0xCF9DDB47u, 0, 0, 1, 0);
}

// Finds the door-table entry whose first word equals key (NULL if none).
static u8* RobotDoorFind(u32 key) {
    for (s32 i = 0; i < 3; i++) {
        u8* e = lbl_803C6A58[i];
        if (*(u32*)e == key) return e;
    }
    return NULL;
}

// 0x80091F6C
// Door opening: sets the matched door entry's state to 0.
void RobotEvolutionExperiment_DoorOpenning(u8* p) {
    u8* e = RobotDoorFind(*(u32*)(p + 0x10));
    if (e == NULL) return;
    *(s32*)(e + 0x4) = 0;
}

// 0x80091FCC
// Door open: sets the matched door entry's state to 1.
void RobotEvolutionExperiment_DoorOpen(u8* p) {
    u8* e = RobotDoorFind(*(u32*)(p + 0x10));
    if (e == NULL) return;
    *(s32*)(e + 0x4) = 1;
}

// 0x8009202C
// Door closing: sets the matched door entry's state to 2.
void RobotEvolutionExperiment_DoorClosing(u8* p) {
    u8* e = RobotDoorFind(*(u32*)(p + 0x10));
    if (e == NULL) return;
    *(s32*)(e + 0x4) = 2;
}

// 0x8009208C
// Door closed: sets the matched door entry's state to 3.
void RobotEvolutionExperiment_DoorClosed(u8* p) {
    u8* e = RobotDoorFind(*(u32*)(p + 0x10));
    if (e == NULL) return;
    *(s32*)(e + 0x4) = 3;
}

// 0x8009210C
// Initialises a record: index/state fields set to -1 or 0, timer at +0x38 set to lbl_805FDB08.
void fn_8009210C(u8* p) {
    *(s32*)(p + 0x4) = 0;
    *(s32*)(p + 0x0) = -1;
    *(s32*)(p + 0x8) = -1;
    *(s32*)(p + 0xC) = 0;
    *(s32*)(p + 0x10) = 0;
    *(u8*)(p + 0x14) = 0;
    *(s32*)(p + 0x18) = 0;
    *(s32*)(p + 0x1C) = 0;
    *(s32*)(p + 0x20) = 0;
    *(s32*)(p + 0x24) = -1;
    *(s32*)(p + 0x28) = -1;
    *(s32*)(p + 0x2C) = 0;
    *(s32*)(p + 0x30) = 0;
    *(u8*)(p + 0x34) = 0;
    *(f32*)(p + 0x38) = lbl_805FDB08;
    *(s32*)(p + 0x44) = -1;
}

// 0x8009215C
// Destructor: tears down the record, frees it with operator_delete when del > 0. Returns p.
u8* fn_8009215C(u8* p, s32 del) {
    if (p != NULL) {
        fn_800923DC(p);
        if (del > 0) operator_delete(p);
    }
    return p;
}
}
