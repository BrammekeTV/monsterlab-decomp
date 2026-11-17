#pragma once
// Characters / scripted entities (partial layout, grown as functions are decompiled).
#include "engine/math.h"

// Path follower used by patrolling characters (Character::path).
struct PathState {
    f32  speed;        // 0x00
    u8   reversed;     // 0x04
    u8   pad05[3];
    s32  state;        // 0x08: 0 = stopped, 1 = moving, 2 = paused
    u8   pad0C[0xC];
    Vec3 position;     // 0x18
};
struct PathFollower {
    PathState* s;
};

// Scene node of a character (Character::def).
struct CharacterDef {
    Vec3 pos;          // 0x00
    Quat rot;          // 0x0C
    u32  flags;        // 0x1C: 0x10 = moving character, 0x2000 = animated, 0x4000 = combat-only, ...
    u32  stateFlags;   // 0x20: 0x1 = active, 0x80 = dead/removed, 0x400000 = boss
};

struct Character {
    void**        vtbl;          // 0x000
    CharacterDef* def;           // 0x004
    u8            pad008[0x1C];
    f32           dt;            // 0x024: frame time for this update
    u8            pad028[4];
    s32           id;            // 0x02C
    u8            pad030[0x680];
    f32           hazardTimer;   // 0x6B0
    s32           eventTarget;   // 0x6B4
    u8            pad6B8[0x180];
    u32           flags838;      // 0x838
    u8            pad83C[8];
    Vec3          velocity;      // 0x844
    u8            pad850[0x2C];
    s32           patrolMode;    // 0x87C: 0 = off, 3 = follow target node, 4 = follow character, else route
    s32           routeLoop;     // 0x880: 0 = loop, 1 = ping-pong, 2 = once, 3 = random at junctions
    s32*          route;         // 0x884: up to 64 junction exits, -1 terminated
    s32           patrolTarget;  // 0x888
    s32           routeIndex;    // 0x88C
    s32           routeLength;   // 0x890
    u8            routeForward;  // 0x894
    u8            unk895;        // 0x895
    u8            pad896[0x142];
    s32           unk9D8;        // 0x9D8
    u8            pad9DC[8];
    f32           waitTimer;     // 0x9E4
    Vec3          targetPos;     // 0x9E8
    f32           pathSpeed;     // 0x9F4
    u8            pad9F8[8];
    PathFollower* path;          // 0xA00
    u8            padA04[0x1DB];
    u8            unkBDF;        // 0xBDF
    u8            padBE0[0x60];
    f32           unkC40;        // 0xC40
    f32           unkC44;        // 0xC44
    u8            padC48[0x38];
    u8            unkC80;        // 0xC80
    u8            padC81;
    u8            unkC82;        // 0xC82
};

// Context passed to every script/AI state callback; the entity is at +0x10.
struct ScriptCtx {
    u8         pad00[0x10];
    Character* chr;            // 0x10
};

// CodeWarrior virtual call through the raw vtable (byte offset of the slot).
#define VCALL(obj, off, type) ((type)((obj)->vtbl[(off) / 4]))

typedef void (*CharKillFn)(Character*);   // vtable + 0x58

extern "C" {
void PathSetMoving(PathFollower* p, BOOL on);         // 0x800D5D5C
void PathSetPaused(PathFollower* p, BOOL on);         // 0x800D5DA4
BOOL PathIsPaused(PathFollower* p);                   // 0x800D5DCC
void PathReverse(PathFollower* p);                    // 0x800D6A00
void PathSetSpeed(PathFollower* p, f32 speed);        // 0x800D6A18
Vec3* PathGetPosition(PathFollower* p);               // 0x800D5718
BOOL PathAtWaypoint(PathFollower* p);                 // 0x800D56E0
u32  PathGetExits(PathFollower* p, int a);            // 0x800D5850: bitmask of usable exits (4)
BOOL PathExitBlocked(PathFollower* p, s32 exit);      // 0x800D6998
void PathTakeExit(PathFollower* p, s32 exit, int a);  // 0x800D675C
s32  PathFindNode(PathFollower* p, s32 node, int a);  // 0x800D648C
s32  PathFindToward(PathFollower* p, PathFollower* other, int a);   // 0x800D6534
}
