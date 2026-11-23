// Missing-function batch 103: movie-player command flush, UI/world helpers, rbtree erase fixup.
#include <math.h>
#include <string.h>
#include "types.h"
#include "game/part.h"
#include "engine/math.h"

typedef f32 Scalar;

extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" void* gLab;               // 0x805FBBC8
extern "C++" s32 gScreenWidth;         // 0x805F9388
extern "C++" s32 gScreenHeight;        // 0x805F938C
extern "C++" void* gScreenSets;        // 0x805FB654
extern "C++" s32 gZoneResourceSize;    // 0x805FB94C

extern "C" {
u8* GetGlobalSettings(void);
void LabQuestFinished(void* lab, s32 id);
void fn_8022667C(u8* s, const u8* spec, void** parts);   // progress.cpp
void fn_802268BC(u8* s, const u8* spec);                 // progress.cpp
s32 RandomUpTo(s32 max);
s32 SaveUnlockReward(u8* s, const u8* reward, BOOL notify);
s32 SpawnEffectAt(void* fxMgr, u32 effect, const f32 pos[4], u8** owner);
u32 OSGetTick(void);
int SysSprintf(char* buf, const char* fmt, ...);
void* EventPost(s32 id, void* sender);
void FriendListSort(BOOL a0);
void MatchupCleanup(void);
void HideAlertBox(void);
void SetResponse(u32 a3);
u64 fn_802FB094(u8* rec);                                    // own friend code
s32 fn_802E63E4(u8* rec, void* code, u32* state);            // 0x802E63E4
s32 fn_802E9AB0(u8* p, s32 a, s32 b, s32 c, void* cb, s32 d, s32 e, s32 f);
void fn_8013A780(s32 a, s32 b);                              // matchup callback
void fn_8013AFF0(void);
void OSLockMutex(u8* p);
void OSUnlockMutex(u8* p);
s32 fn_801F525C(void);
void fn_801F357C(void);
s32 fn_801F57C4(void);
void fn_801F57B8(void);
void fn_801F57AC(void);
void StickRead(s32 channel, f32* x, f32* y);   // 0x80147704
f32 fn_80147800(s32 id);                      // 0x80147800: analogue value of input id
void* fn_801DA798(void* p);
u8* fn_80196B24(u8* obj);   // returns the transform at the top of the stack (leaf wrongly says u32)
void Vec3Copy(Vec3* out, const Vec3* v);
void Vec3MulScalar(Vec3* out, const Vec3* v, const Scalar* s);
void Vec3Negate(Vec3* out, const Vec3* v);
void Vec3SetX(Vec3* v, f32 x);
void Vec3SetY(Vec3* v, f32 y);
void Vec3SetZ(Vec3* v, f32 z);
void Vec3SetZero(Vec3* out);
void ScalarSet(Scalar* out, f32 v);
void MtxFromLocatorRotation(f32 out[12], const void* rot);
void fn_80052638(void);   // 0x80052638 (empty leaf; the caller passes the vector)
void fn_800526D0(u8* out);
void fn_8005535C(u8* dst, const u8* src);
f32 fn_800553C0(u8* mtx, const Vec3* v);
void fn_800554DC(u8* out, const u8* a, const u8* b);
void fn_800557B4(u8* out);
f32 fn_80058BD4(Scalar* out, const Scalar* in);
void fn_8016EB34(Scalar* out, const Vec3* v);
void fn_8016EB40(Scalar* out, const Vec3* v);
void fn_8016F778(Scalar* out, const Vec3* v);
void fn_8016F848(Vec3* out, const Scalar* s);
void fn_800676D4(Vec3* out, const Scalar* s);
s32 fn_800677A0(const f32* v);
void fn_8019C6DC(u8* mtx, const Vec3* v);
void fn_8019C710(Vec3* out, const Scalar* s);
void fn_8019C71C(u8* out, const u8* src);
void fn_8018DEF4(void* sets);
void fn_801D65C0(void);
void fn_80199C30(void* obj);
void fn_8019BFF8(void* obj, void* cb, void* arg);
void* UIWidgetFindChild(void* widget, u32 hash);
s32 fn_80199094(void* obj);   // src/engine/small_8019c.cpp
s32 fn_80199100(void* obj);   // src/engine/small_8019c.cpp
s32 fn_80198290(u8* a3, s32 a4);
s32 fn_8019829C(u8* a3, f32 f1);
s32 fn_801223C0(u8* self, s32 unused, u8** info);   // 0x801223C0
u8* fn_801ED684(u8* handle);
f64 fn_8033F1F8(f64 x);   // 0x8033F1F8, takes the ratio in f1, returns an angle in f1
f32 fn_8033F1FC(f32 y, f32 x);
u32 fn_80143974(s32 seed, void* data, u32 len);
void fn_801F5B30(u8* buf);
u8* fn_801F5BB8(u8* p, s32 id, s32 a, s32 b, s32 c, s32 d);
void fn_801F5E48(void* a, s32 b);
void fn_801F5E4C(void* a, s32 b);

extern u8 gWorldState[];
extern u8 lbl_803C4298[];
extern u8 lbl_80375C48[];       // "Match Friend", "Match Anybody", "No idea!" at +0, +0x10, +0x20
extern u8 lbl_80556AC8[];       // connection lock object
extern u8 lbl_80556E80[];       // player data block
extern u32 lbl_803F0F30[];      // per-slot state flags
extern u32 lbl_805F9030;
extern s32 lbl_805F9028;
extern s32 lbl_805FB378;
extern s32 lbl_805FB97C;
extern const char lbl_805F9038[];   // "Offline"
extern const char lbl_805F9040[];   // "Online"
extern const char lbl_805F9048[];   // "Playing"
extern const char lbl_805F9050[];   // "Client"
extern const char lbl_805F9058[];   // "Server"
extern u8 lbl_803E6C20[];
extern u8 lbl_8047C760[];
extern u8 lbl_804DD568[];
extern s32 lbl_804FB698[];
extern u8 lbl_804FB5E0[];
extern f32 lbl_804FDA40[];   // quad corners (4 x,y pairs)
extern f32 lbl_804FDA60[];   // 3x4 transform
extern u8 lbl_805F98B8[];
extern void* lbl_805FB658;
extern void* lbl_805FB670;
extern f32 lbl_805FB91C;
extern u8* lbl_805FB95C;
extern void* lbl_805FB964;
extern s32 lbl_805FB970;
extern s32 lbl_805FB974;
extern s32 lbl_805FB990;
extern f32 lbl_805FB99C;
extern s32 lbl_805FB9D8;
extern u32 lbl_805FB9E4;
extern s32 lbl_805FB9E8;
extern s32 lbl_805FBA70;
extern u32 lbl_805FBA74;
extern u8* lbl_805FBA78;
extern u8 lbl_805FBA7C;
}

typedef s32 (*MovieFn0)(void*);
typedef s32 (*MovieFnSend)(void*, void*, s32);
typedef void (*CamFloatFn)(void*, f32);
typedef s32 (*ObjCmdFn)(void*, s32);
typedef s32 (*VtblFn0)(void*);

struct Mtx34 { f32 m[12]; };
static inline u8* MP(Mtx34& m) { return (u8*)&m; }

static u8* CamObj(void) { return *(u8**)(lbl_803C4298 + 0x4); }

// Sum of the 13 per-category entity counters (+0x34..+0x64) of the global settings.
static s32 SettingsEntityTotal(void) {
    u8* g = GetGlobalSettings();
    s32 sum = 0;
    for (s32 off = 0x34; off <= 0x64; off += 4) sum += *(s32*)(g + off);
    return sum;
}

// Ring-buffer membership test used by the movie packet queue (head at +8, tail at +0xC,
// capacity at +0x10 of lbl_804DD568): true when idx lies between head and tail, wrapping.
static s32 RingContains(s32 head, s32 tail, s32 cap, s32 idx) {
    if (idx < 0 || idx >= cap) return 0;
    s32 v = idx;
    if (tail < head) {
        if (idx <= tail) v = idx + cap;
        tail += cap;
    }
    if (head > v) return 0;
    if (v > tail) return 0;
    return 1;
}

// Red-black tree node used by fn_800A2AEC: left at +0, right at +4, parent pointer with the colour
// in bit 0 (1 = red) at +8.
static inline u8*& NodeLeft(u8* n) { return *(u8**)(n + 0x0); }
static inline u8*& NodeRight(u8* n) { return *(u8**)(n + 0x4); }
static inline uintptr_t& NodeWord(u8* n) { return *(uintptr_t*)(n + 0x8); }
static inline u8* NodeParent(u8* n) { return (u8*)(NodeWord(n) & ~(uintptr_t)1); }
static inline BOOL NodeIsRed(u8* n) { return (NodeWord(n) & 1) == 1; }
static inline void NodeSetBlack(u8* n) { NodeWord(n) &= ~(uintptr_t)1; }
static inline void NodeSetRed(u8* n) { NodeWord(n) |= 1; }

// Left rotation of x around its right child y.
static void NodeRotateLeft(u8* x, u8* y) {
    NodeRight(x) = NodeLeft(y);
    u8* c = NodeLeft(y);
    if (c != NULL) NodeWord(c) = (uintptr_t)x | (NodeWord(c) & 1);
    NodeWord(y) = (NodeWord(y) & 1) | (NodeWord(x) & ~(uintptr_t)1);
    u8* pp = NodeParent(x);
    if (x == NodeLeft(pp)) NodeLeft(pp) = y;
    else NodeRight(pp) = y;
    NodeLeft(y) = x;
    NodeWord(x) = (uintptr_t)y | (NodeWord(x) & 1);
}

// Right rotation of x around its left child y.
static void NodeRotateRight(u8* x, u8* y) {
    NodeLeft(x) = NodeRight(y);
    u8* c = NodeRight(y);
    if (c != NULL) NodeWord(c) = (uintptr_t)x | (NodeWord(c) & 1);
    NodeWord(y) = (NodeWord(y) & 1) | (NodeWord(x) & ~(uintptr_t)1);
    u8* pp = NodeParent(x);
    if (x == NodeLeft(pp)) NodeLeft(pp) = y;
    else NodeRight(pp) = y;
    NodeRight(y) = x;
    NodeWord(x) = (uintptr_t)y | (NodeWord(x) & 1);
}

// Input-source value helpers (dead zone 0.005): positive-only, negative-only, and dead-zoned.
static f32 InputPositive(f32 v) { return v > 0.005f ? v : 0.0f; }
static f32 InputNegative(f32 v) { return v >= -0.005f ? 0.0f : -v; }
static f32 InputNegativeStrict(f32 v) {
    if (v > -0.005f) v = 0.0f;
    return v < 0.0f ? -v : 0.0f;
}
static f32 InputDeadZone(f32 v) {
    if (v < 0.005f && v > -0.005f) v = 0.0f;
    return v;
}

// True when the movie/session object exists, is active and reports vtable slot 0x34 as set.
static BOOL SessionObjectReady(void) {
    if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL) {
        if ((*(MovieFn0*)(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0) return TRUE;
    }
    return FALSE;
}

struct MatchupStartArg {
    u8 slot;
    u8 pad[3];
    u32 state;
    u64 code;
};

extern "C" {

// 0x8001FCA8
// Updates one input source record: p[0] = source id (channel), p[8] = source kind, result stored
// as a float at +4. Kinds 0-13 are digital buttons (analogue value above 0.005 gives 1, else 0);
// 14-21 are stick half-axes (left/right/up/down for both sticks, giving the magnitude on that
// side); 22 does nothing; 23-26 are full stick axes with a 0.005 dead zone; 27 and 28 are the
// negated stick angle (atan2 of y and x).
void fn_8001FCA8(u8* p) {
    s32 channel = *(s32*)(p + 0x0);
    f32 x, y;
    switch (*(u32*)(p + 0x8)) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6:
    case 7: case 8: case 9: case 10: case 11: case 12: case 13:
        *(f32*)(p + 0x4) = fn_80147800(channel) > 0.005f ? 1.0f : 0.0f;
        break;
    case 14:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputNegative(x);
        break;
    case 15:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputPositive(x);
        break;
    case 16:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputNegativeStrict(y);
        break;
    case 17:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputPositive(y);
        break;
    case 18:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputNegative(x);
        break;
    case 19:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputPositive(x);
        break;
    case 20:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputNegativeStrict(y);
        break;
    case 21:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputPositive(y);
        break;
    case 23:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputDeadZone(x);
        break;
    case 24:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputDeadZone(y);
        break;
    case 25:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputDeadZone(x);
        break;
    case 26:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = InputDeadZone(y);
        break;
    case 27:
    case 28:
        StickRead(channel, &x, &y);
        *(f32*)(p + 0x4) = -fn_8033F1FC(y, x);
        break;
    default:
        break;
    }
}

// 0x8013A0A4
// Matchup callback: slot a's connection event b (0 offline, 1 online, 2 playing, 3 match anybody,
// 4 match friend, 5 client, 6 server). Records the event name and slot state in lbl_803F0F30[a]
// (0x10 offline, 2 online, 8 playing, 4 match friend), runs the matchup clean-up when the current
// slot goes offline/online, starts a match with a friend whose code matches our own, or answers
// an invitation when playing, then refreshes the friend list (or re-sorts it when lbl_805F9028 == 1).
void GetFriendStatusString(s32 a, s32 b) {
    char text[0x100];
    s32 slot = a;
    OSLockMutex(lbl_80556AC8);
    switch ((u32)b) {
    case 0:
        SysSprintf(text, lbl_805F9038);
        lbl_803F0F30[slot] = 0x10;
        if (lbl_805F9030 == (u32)slot && fn_801F57C4() != 0) {
            fn_801F57B8();
            MatchupCleanup();
        }
        break;
    case 1:
        SysSprintf(text, lbl_805F9040);
        lbl_803F0F30[slot] = 2;
        if (lbl_805F9030 == (u32)slot && fn_801F57C4() != 0) {
            fn_801F57B8();
            MatchupCleanup();
        }
        break;
    case 2:
        lbl_803F0F30[slot] = 8;
        SysSprintf(text, lbl_805F9048);
        if (SessionObjectReady() && lbl_805F9030 != 0x40 && lbl_805FB378 != 0) {
            BOOL inCall = SessionObjectReady() && lbl_805FB97C != 0;
            if (inCall) {
                SetResponse(0x099E8EEDu);
                HideAlertBox();
            }
            if (fn_801F525C() == 0) fn_801F357C();
            u8* ev = (u8*)EventPost(0x36, NULL);
            if (ev != NULL) {
                u64 own = fn_802FB094(lbl_80556E80);
                *(u32*)(ev + 0x8) = (u32)(own >> 32);
                *(u32*)(ev + 0xC) = (u32)own;
            }
        }
        break;
    case 3:
        SysSprintf(text, (const char*)(lbl_80375C48 + 0x10));
        break;
    case 4:
        SysSprintf(text, (const char*)(lbl_80375C48 + 0x0));
        lbl_803F0F30[slot] = 4;
        if (!SessionObjectReady() && lbl_805FB378 == 0 && fn_801F57C4() == 0) {
            MatchupStartArg arg;
            arg.state = 0;
            arg.code = 0;
            u8* rec = lbl_80556E80 + slot * 0xC + 0x40;
            fn_802E63E4(rec, NULL, &arg.state);
            if (arg.state == 8) fn_802E63E4(rec, &arg.code, &arg.state);
            if (arg.code == fn_802FB094(lbl_80556E80)) {
                lbl_805F9030 = (u32)slot;
                fn_801F57AC();
                arg.slot = (u8)slot;
                fn_802E9AB0((u8*)&arg, 1, 2, 0, (void*)fn_8013A780, 0, 0, 0);
            }
        }
        break;
    case 5:
        SysSprintf(text, lbl_805F9050);
        break;
    case 6:
        SysSprintf(text, lbl_805F9058);
        break;
    default:
        SysSprintf(text, (const char*)(lbl_80375C48 + 0x20));
        break;
    }
    if (lbl_805F9028 == 1) {
        FriendListSort(1);
    } else {
        lbl_803F0F30[slot] = lbl_803F0F30[slot] | 1;
        fn_8013AFF0();
    }
    OSUnlockMutex(lbl_80556AC8);
}

// 0x800A2AEC
// Red-black insert fixup: colours node red, then recolours/rotates up the tree until the parent is
// black or the root is reached, and finally paints the (possibly new) root black.
void fn_800A2AEC(u8* node, u8* root) {
    NodeSetRed(node);
    while (node != root && NodeIsRed(NodeParent(node))) {
        u8* p = NodeParent(node);
        u8* g = NodeParent(p);
        u8* uncle = NodeLeft(g);
        if (p == uncle) {
            uncle = NodeRight(g);
            if (uncle != NULL && NodeIsRed(uncle)) {
                NodeSetBlack(p);
                NodeSetBlack(uncle);
                node = NodeParent(NodeParent(node));
                NodeSetRed(node);
                continue;
            }
            u8* sib = NodeRight(p);
            if (node == sib) {
                node = p;
                if (root == p) root = sib;
                NodeRotateLeft(p, sib);
            }
            NodeSetBlack(NodeParent(node));
            u8* g2 = NodeParent(NodeParent(node));
            NodeSetRed(g2);
            u8* l = NodeLeft(g2);
            if (root == g2) root = l;
            NodeRotateRight(g2, l);
        } else {
            if (uncle != NULL && NodeIsRed(uncle)) {
                NodeSetBlack(p);
                NodeSetBlack(uncle);
                node = NodeParent(NodeParent(node));
                NodeSetRed(node);
                continue;
            }
            u8* sib = NodeLeft(p);
            if (node == sib) {
                node = p;
                if (root == p) root = sib;
                NodeRotateRight(p, sib);
            }
            NodeSetBlack(NodeParent(node));
            u8* g2 = NodeParent(NodeParent(node));
            NodeSetRed(g2);
            u8* r = NodeRight(g2);
            if (root == g2) root = r;
            NodeRotateLeft(g2, r);
        }
    }
    NodeSetBlack(root);
}

// 0x800E5684
// Recomputes the cached aim data of the record: unit vectors (+0x68, +0x74) from the vectors of the
// entities at +0xB0 / +0xB4, the direction from entity +0xA8 to entity +0xAC, its heading (+0x98)
// and pitch (+0x9C) angles, the point 0.8 along that direction from entity +0xA8 (+0x80), a zero
// vector (+0x8C) and the heading again at +0xA0. (Returns nothing; r3 is left over from a call.)
u8* fn_800E5684(u8* p) {
    u8* e = fn_801ED684(p + 0xB0);
    f32 vx = *(f32*)(e + 0x8), vy = *(f32*)(e + 0xC), vz = *(f32*)(e + 0x10);
    f32 inv = 1.0f / sqrtf(vz * vz + (vx * vx + vy * vy));
    *(f32*)(p + 0x68) = vx * inv;
    *(f32*)(p + 0x6C) = vy * inv;
    *(f32*)(p + 0x70) = vz * inv;

    e = fn_801ED684(p + 0xB4);
    vx = *(f32*)(e + 0x8);
    vy = *(f32*)(e + 0xC);
    vz = *(f32*)(e + 0x10);
    inv = 1.0f / sqrtf(vz * vz + (vx * vx + vy * vy));
    *(f32*)(p + 0x74) = vx * inv;
    *(f32*)(p + 0x78) = vy * inv;
    *(f32*)(p + 0x7C) = vz * inv;

    u8* from = fn_801ED684(p + 0xA8);
    u8* to = fn_801ED684(p + 0xAC);
    f32 dx = *(f32*)(to + 0x8) - *(f32*)(from + 0x8);
    f32 dy = *(f32*)(to + 0xC) - *(f32*)(from + 0xC);
    f32 dz = *(f32*)(to + 0x10) - *(f32*)(from + 0x10);
    inv = 1.0f / sqrtf(dz * dz + (dx * dx + dy * dy));
    f32 ux = dx * inv, uy = dy * inv, uz = dz * inv;

    *(f32*)(p + 0x98) = fn_8033F1FC(uy, ux);
    f32 len2 = uz * uz + (ux * ux + uy * uy);
    f32 len = 0.0f;
    if (!(len2 <= 0.0f)) len = sqrtf(len2);
    if (len < 0.001f) {
        *(f32*)(p + 0x9C) = 0.0f;
        *(f32*)(p + 0x98) = 0.0f;
    } else {
        f32 ratio = uz / len;
        if (ratio < -1.0f) *(f32*)(p + 0x9C) = -3.1415927f;
        if (ratio > 1.0f) *(f32*)(p + 0x9C) = 3.1415927f;
        else *(f32*)(p + 0x9C) = (f32)fn_8033F1F8((f64)ratio);
    }

    u8* origin = fn_801ED684(p + 0xA8);
    f32 ox = *(f32*)(origin + 0x8), oy = *(f32*)(origin + 0xC), oz = *(f32*)(origin + 0x10);
    *(f32*)(p + 0x80) = ox + ux * 0.8f;
    *(f32*)(p + 0x84) = oy + uy * 0.8f;
    *(f32*)(p + 0x88) = oz + uz * 0.8f;
    *(f32*)(p + 0x8C) = 0.0f;
    *(f32*)(p + 0x90) = 0.0f;
    *(f32*)(p + 0x94) = 0.0f;
    *(f32*)(p + 0xA0) = *(f32*)(p + 0x98);
    return origin;  // r3 is left holding the last lookup result; callers pass it on
}

// 0x801225F4
// Blast at pos: records pos in the 0x1C-byte blast slot lbl_803E6C20[clamp(power)-1] (slot 0 when
// power is outside 1..4), spawns the effect (when non-zero) and calls fn_801223C0 on every
// entity (not `self`) of every zone (plus zone 0) whose flags (+0x1C & 0x50, +0x20 & 2) allow
// it and which is within the slot's radius (+0xC). Finally sets three camera values scaled by power.
void fn_801225F4(u8* self, u8* pos, s32 effect, s8 power) {
    u8* slot;
    if (power > 0 && power <= 4) slot = lbl_803E6C20 + (power - 1) * 0x1C;
    else slot = lbl_803E6C20;
    if (slot != NULL) {
        *(f32*)(slot + 0x0) = *(f32*)(pos + 0x0);
        *(f32*)(slot + 0x4) = *(f32*)(pos + 0x4);
        *(f32*)(slot + 0x8) = *(f32*)(pos + 0x8);
    }
    if (effect != 0) {
        f32 fx[4];
        fx[0] = *(f32*)(pos + 0x0);
        fx[1] = *(f32*)(pos + 0x4);
        fx[2] = *(f32*)(pos + 0x8);
        fx[3] = 0.0f;
        SpawnEffectAt(lbl_8047C760, (u32)effect, fx, 0);
    }
    for (s32 zoneIdx = 0, off = 0; zoneIdx < SettingsEntityTotal() + 1; zoneIdx++, off += 0x30) {
        s32 zone = -1;
        if (zoneIdx == SettingsEntityTotal()) {
            zone = 0;
        } else {
            u8* e = *(u8**)(gWorldState + 0xB0) + off;
            if (*(s32*)(e + 0x4) == 6) zone = *(s32*)(e + 0x8);
        }
        if (zone < 0) continue;
        u8* z = *(u8**)(gWorldState + 0x1A4) + zone * gZoneResourceSize;
        s32 total = *(s32*)(z + 0x94) + *(s32*)(z + 0x98);
        for (s32 i = 0, idx = 0; i < total; i++, idx += 4) {
            z = *(u8**)(gWorldState + 0x1A4) + zone * gZoneResourceSize;
            s32 first = *(s32*)(z + 0x94);
            u8* obj;
            if (i < first) obj = *(u8**)(*(u8**)(z + 0x120) + idx);
            else obj = *(u8**)(*(u8**)(z + 0x12C) + (i - first) * 4);
            if (obj == NULL || self == obj) continue;
            u8* data = *(u8**)(obj + 0x4);
            if ((*(u32*)(data + 0x1C) & 0x50) == 0) continue;
            if ((*(u32*)(data + 0x20) & 2) == 0) continue;
            f32 dx = *(f32*)(pos + 0x0) - *(f32*)(data + 0x0);
            f32 dy = *(f32*)(pos + 0x4) - *(f32*)(data + 0x4);
            f32 dz = *(f32*)(pos + 0x8) - *(f32*)(data + 0x8);
            f32 radius = *(f32*)(slot + 0xC);
            if (dz * dz + (dx * dx + dy * dy) < radius * radius) fn_801223C0(obj, zone, &self);
        }
    }
    (*(CamFloatFn*)(*(u8**)CamObj() + 0xA0))(CamObj(), 0.25f * (f32)power);
    (*(CamFloatFn*)(*(u8**)CamObj() + 0xA4))(CamObj(), 0.025f);
    (*(CamFloatFn*)(*(u8**)CamObj() + 0xA8))(CamObj(), 1.0f);
}

// 0x80198C78
// Closes every widget of the screen set (widget list at +0x40, count at +0x60), topmost first:
// pops it, clears its callbacks (fn_80199094 / fn_80199100 / fn_80199C30) and calls fn_801D65C0
// when no remaining widget is a blocking one (type "loading", or 0x1B6732BB with flag 2 on its
// child 0x8DC6BEC1). The new top widget's value at +0x148 is handed to lbl_805FB670. Returns 0.
s32 fn_80198C78(u8* set) {
    while (*(s32*)(set + 0x60) != 0) {
        if (*(s32*)(set + 0x60) > 0) {
            fn_8018DEF4(gScreenSets);
            s32 top = *(s32*)(set + 0x60) - 1;
            *(s32*)(set + 0x60) = top;
            u8* closing = *(u8**)(set + 0x40 + top * 4);
            s32 blocking = 0;
            for (s32 i = 0; i < *(s32*)(set + 0x60); i++) {
                u8* w = *(u8**)(set + 0x40 + i * 4);
                if (w != NULL) {
                    u32 type = *(u32*)w;
                    if (type == 0x6825C6AEu) {  // "loading"
                        blocking = 1;
                    } else if (type == 0x1B6732BBu) {
                        u8* child = (u8*)UIWidgetFindChild(w, 0x8DC6BEC1u);
                        if (*(u32*)(child + 0x64) & 2) blocking = 1;
                    }
                }
                if (blocking) break;
            }
            if (blocking == 0) fn_801D65C0();
            fn_8019BFF8(closing, (void*)fn_80199094, NULL);
            fn_8019BFF8(closing, (void*)fn_80199100, NULL);
            fn_80199C30(closing);
            s32 left = *(s32*)(set + 0x60);
            if (left > 0) {
                u8* next = *(u8**)(set + 0x3C + left * 4);
                fn_80198290((u8*)lbl_805FB670, 2);
                fn_8019829C((u8*)lbl_805FB670, *(f32*)(next + 0x148));
            }
        }
        *(s32*)((u8*)gScreenSets + 0x74) = 0;
    }
    return 0;
}

// 0x8019C348
// Rebuilds the object's transforms: +0xF0 = T(-pos...) * R * S matrices built from the scale
// (+0x74), rotation (+0x90), and position (+0x68) vectors and the offset at +0x9C; +0xC0 is
// the inverse transform (built from reciprocal scales; chained with the parent's +0xC0 when the
// parent at +0x128 exists). Then tells every child (list at +0x120, next at +0x124) and the
// object itself, through vtable slots 0x2C / 0x30 of the vtable at +0x138, to update.
void fn_8019C348(u8* self) {
    Mtx34 mA, mB, mC, mD, t;
    Mtx34 m1E0, m210, m240, m180, m90, mC0, mF0;
    Vec3 v84, v78, v6C, v60, v54, v48, v3C;
    Scalar s38, s34, s30, s2C, s28, s24, s20, s1C, s18, s14, s10, sC, s8;

    fn_800526D0(MP(mA));
    fn_800526D0(MP(mB));
    fn_800526D0(MP(mC));
    fn_800526D0(MP(mD));
    fn_80052638();
    fn_800557B4(MP(t));
    fn_8005535C(MP(mA), MP(t));
    fn_8019C6DC(MP(mA), (const Vec3*)(self + 0x74));
    Vec3Copy(&v84, (const Vec3*)(self + 0x9C));
    fn_800557B4(MP(t));
    fn_8005535C(MP(mB), MP(t));
    Vec3Negate(&v6C, (const Vec3*)(self + 0x9C));
    fn_800553C0(MP(mB), &v6C);
    ScalarSet(&s38, 0.63661975f);
    Vec3MulScalar(&v60, (const Vec3*)(self + 0x90), &s38);
    MtxFromLocatorRotation(mC.m, &v60);
    fn_800557B4(MP(t));
    fn_8005535C(MP(mD), MP(t));
    fn_800553C0(MP(mD), (const Vec3*)(self + 0x68));
    fn_800554DC(MP(m1E0), MP(mB), MP(mA));
    fn_800554DC(MP(m210), MP(m1E0), MP(mC));
    fn_800554DC(MP(m240), MP(m210), MP(mD));
    fn_8005535C(self + 0xF0, MP(m240));
    fn_800557B4(MP(t));
    fn_8005535C(self + 0xC0, MP(t));
    u8* parent = *(u8**)(self + 0x128);
    if (parent != NULL) {
        fn_800554DC(MP(m180), parent + 0xC0, self + 0xC0);
        fn_8005535C(self + 0xC0, MP(m180));
    }
    fn_800557B4(MP(t));
    fn_8005535C(MP(mA), MP(t));
    Vec3SetZero(&v78);

    fn_8016EB34(&s34, (const Vec3*)(self + 0x74));
    if (fn_800677A0((const f32*)&s34) != 0) {
        Vec3SetX(&v78, 10000.0f);
    } else {
        fn_8016EB34(&s2C, (const Vec3*)(self + 0x74));
        fn_80058BD4(&s30, &s2C);
        fn_8019C710(&v78, &s30);
    }
    fn_8016EB40(&s28, (const Vec3*)(self + 0x74));
    if (fn_800677A0((const f32*)&s28) != 0) {
        Vec3SetY(&v78, 10000.0f);
    } else {
        fn_8016EB40(&s20, (const Vec3*)(self + 0x74));
        fn_80058BD4(&s24, &s20);
        fn_8016F848(&v78, &s24);
    }
    fn_8016F778(&s1C, (const Vec3*)(self + 0x74));
    if (fn_800677A0((const f32*)&s1C) != 0) {
        Vec3SetZ(&v78, 10000.0f);
    } else {
        fn_8016F778(&s14, (const Vec3*)(self + 0x74));
        fn_80058BD4(&s18, &s14);
        fn_800676D4(&v78, &s18);
    }
    fn_8019C6DC(MP(mA), &v78);
    fn_800557B4(MP(t));
    fn_8005535C(MP(mB), MP(t));
    fn_800553C0(MP(mB), (const Vec3*)(self + 0x9C));
    ScalarSet(&sC, -1.0f);
    ScalarSet(&s10, 0.63661975f);
    Vec3MulScalar(&v48, (const Vec3*)(self + 0x90), &s10);
    Vec3MulScalar(&v54, &v48, &sC);
    MtxFromLocatorRotation(mC.m, &v54);
    ScalarSet(&s8, -1.0f);
    Vec3MulScalar(&v3C, (const Vec3*)(self + 0x68), &s8);
    fn_800553C0(MP(mD), &v3C);
    fn_800554DC(MP(m90), MP(mD), MP(mC));
    fn_800554DC(MP(mC0), MP(m90), MP(mA));
    fn_800554DC(MP(mF0), MP(mC0), MP(mB));
    fn_8019C71C(self + 0xC0, MP(mF0));

    for (u8* child = *(u8**)(self + 0x120); child != NULL; child = *(u8**)(child + 0x124)) {
        (*(ObjCmdFn*)(*(u8**)(child + 0x138) + 0x2C))(child, 0x28);
    }
    (*(ObjCmdFn*)(*(u8**)(self + 0x138) + 0x30))(self, 8);
}

// 0x8019DC58
// Composes the object's local 3x4 transform (+0xF0..+0x11C) with the matrix returned by
// fn_80196B24 (render object lbl_805FB658) into +0x17C..+0x1A8, or copies it unchanged when there
// is none. Mirrors the last element (+0x1A8) to +0x5C, then calls vtable slots 0x30 (with 0x10)
// and 0x1C of the vtable at +0x138.
void fn_8019DC58(u8* self) {
    u8* a = fn_80196B24((u8*)lbl_805FB658);
    if (a != NULL) {
        f32 a0 = *(f32*)(a + 0x0), a1 = *(f32*)(a + 0x4), a2 = *(f32*)(a + 0x8);
        f32 a3 = *(f32*)(a + 0xC), a4 = *(f32*)(a + 0x10), a5 = *(f32*)(a + 0x14);
        f32 a6 = *(f32*)(a + 0x18), a7 = *(f32*)(a + 0x1C), a8 = *(f32*)(a + 0x20);
        f32 a9 = *(f32*)(a + 0x24), a10 = *(f32*)(a + 0x28), a11 = *(f32*)(a + 0x2C);
        f32 b0 = *(f32*)(self + 0xF0), b1 = *(f32*)(self + 0xF4), b2 = *(f32*)(self + 0xF8);
        f32 b3 = *(f32*)(self + 0xFC), b4 = *(f32*)(self + 0x100), b5 = *(f32*)(self + 0x104);
        f32 b6 = *(f32*)(self + 0x108), b7 = *(f32*)(self + 0x10C), b8 = *(f32*)(self + 0x110);
        f32 b9 = *(f32*)(self + 0x114), b10 = *(f32*)(self + 0x118), b11 = *(f32*)(self + 0x11C);
        *(f32*)(self + 0x17C) = (a6 * b2) + ((a0 * b0) + (a3 * b1));
        *(f32*)(self + 0x180) = (a7 * b2) + ((a1 * b0) + (a4 * b1));
        *(f32*)(self + 0x184) = (a8 * b2) + ((a2 * b0) + (a5 * b1));
        *(f32*)(self + 0x188) = (a6 * b5) + ((a0 * b3) + (a3 * b4));
        *(f32*)(self + 0x18C) = (a7 * b5) + ((a1 * b3) + (a4 * b4));
        *(f32*)(self + 0x190) = (a8 * b5) + ((a2 * b3) + (a5 * b4));
        *(f32*)(self + 0x194) = (a6 * b8) + ((a0 * b6) + (a3 * b7));
        *(f32*)(self + 0x198) = (a7 * b8) + ((a1 * b6) + (a4 * b7));
        *(f32*)(self + 0x19C) = (a8 * b8) + ((a2 * b6) + (a5 * b7));
        *(f32*)(self + 0x1A0) = a9 + ((a6 * b11) + ((a0 * b9) + (a3 * b10)));
        *(f32*)(self + 0x1A4) = a10 + ((a7 * b11) + ((a1 * b9) + (a4 * b10)));
        *(f32*)(self + 0x1A8) = a11 + ((a8 * b11) + ((a2 * b9) + (a5 * b10)));
    } else {
        for (s32 off = 0; off < 0x30; off += 4) {
            *(f32*)(self + 0x17C + off) = *(f32*)(self + 0xF0 + off);
        }
    }
    *(f32*)(self + 0x5C) = *(f32*)(self + 0x1A8);
    (*(ObjCmdFn*)(*(u8**)(self + 0x138) + 0x30))(self, 0x10);
    (*(VtblFn0*)(*(u8**)(self + 0x138) + 0x1C))(self);
}

// 0x801D9164
// Writes the 3x4 transform of the object (matrix at *(obj+0x34), or the one fn_801DA798 returns
// when the object's kind at +4 is 3) from translation pos (3 floats), scale (3 floats) and
// quaternion quat (x, y, z, w): rows 0..2 are the rotation rows scaled by sx, sy, sz and
// the translation fills +0x24..+0x2C.
void fn_801D9164(u8* obj, u8* pos, u8* scale, u8* quat) {
    u8* dest = *(u8**)(obj + 0x34);
    if (*(s32*)(obj + 0x4) == 3) dest = (u8*)fn_801DA798(obj);

    f32 x = *(f32*)(quat + 0x0), y = *(f32*)(quat + 0x4), z = *(f32*)(quat + 0x8), w = *(f32*)(quat + 0xC);
    f32 ex = 2.0f * x, ey = 2.0f * y, ez = 2.0f * z;
    f32 xx = ex * x, yy = ey * y, zz = ez * z;
    f32 zy = ez * y, xw = ex * w;
    f32 r00 = 1.0f - (yy + zz);
    f32 r11 = 1.0f - (xx + zz);
    f32 r12 = zy + xw;
    f32 r21 = zy - xw;
    f32 r22 = 1.0f - (xx + yy);
    f32 yw = ey * w, zx = ez * x, zw = ez * w, yx = ey * x;
    f32 r02 = zx - yw;
    f32 r20 = zx + yw;
    f32 r01 = yx + zw;
    f32 r10 = yx - zw;

    f32 sx = *(f32*)(scale + 0x0), sy = *(f32*)(scale + 0x4), sz = *(f32*)(scale + 0x8);
    f32 tx = *(f32*)(pos + 0x0), ty = *(f32*)(pos + 0x4), tz = *(f32*)(pos + 0x8);
    f32 zero = 0.0f;

    *(f32*)(dest + 0x00) = (r20 * zero) + ((r00 * sx) + (r10 * zero));
    *(f32*)(dest + 0x04) = (r21 * zero) + ((r01 * sx) + (r11 * zero));
    *(f32*)(dest + 0x08) = (r22 * zero) + ((r02 * sx) + (r12 * zero));
    *(f32*)(dest + 0x0C) = (r20 * zero) + ((r00 * zero) + (r10 * sy));
    *(f32*)(dest + 0x10) = (r21 * zero) + ((r01 * zero) + (r11 * sy));
    *(f32*)(dest + 0x14) = (r22 * zero) + ((r02 * zero) + (r12 * sy));
    *(f32*)(dest + 0x18) = (r20 * sz) + ((r00 * zero) + (r10 * zero));
    *(f32*)(dest + 0x1C) = (r21 * sz) + ((r01 * zero) + (r11 * zero));
    *(f32*)(dest + 0x20) = (r22 * sz) + ((r02 * zero) + (r12 * zero));
    *(f32*)(dest + 0x24) = tx;
    *(f32*)(dest + 0x28) = ty;
    *(f32*)(dest + 0x2C) = tz;
}

// 0x801F401C
// Movie player: flushes the queued 0x400-byte packets to the player (vtable slot 0x18), at most
// three stale ones per call, and sends a freshly built command packet when pending.
void fn_801F401C(void) {
    u32 packet[0x100];
    u8* buf = (u8*)packet;
    s32 sent;
    u8* cur;

    if (lbl_805FB970 == 0) return;
    if (lbl_805FB974 == 0) return;
    if ((*(MovieFn0*)(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) == 0) return;

    if (lbl_805FB9D8 == 0) fn_801F5E4C(lbl_804FB5E0, 3);
    lbl_805FB9D8 = lbl_805FB9D8 + 1;

    s32 head = *(s32*)(lbl_804DD568 + 0x8);
    s32 tail = *(s32*)(lbl_804DD568 + 0xC);
    s32 cap = *(s32*)(lbl_804DD568 + 0x10);
    if (RingContains(head, tail, cap, head)) cur = *(u8**)(lbl_804DD568 + 0x14) + (head << 10);
    else cur = NULL;

    sent = 0;
    s32 timeStep = (s32)lbl_805FB91C;
    if ((f32)(timeStep - lbl_805FB9E8) >= 1.0f) {
        lbl_805FB9E4 = 0;
        lbl_805FB9E8 = timeStep;
    }

    if (lbl_805FB990 != 0 && lbl_805FB9E4 < 9u) {
        fn_801F5B30(buf);
        s32* src = lbl_804FB698;
        for (u32 i = 0; i < (u32)lbl_805FB990; i++) {
            u8* cmd = fn_801F5BB8(buf, 0x7E, 0, 0, 0, 0);
            if (cmd != NULL) *(s32*)(cmd + 0x4) = *src;
            src++;
        }
        lbl_805FB990 = 0;
        *(u32*)(buf + 0x4) = 0;
        *(u32*)(buf + 0x0) = fn_80143974(0, buf + 0x18, *(u32*)(buf + 0x10) - 0x18);
        (*(MovieFnSend*)(*(u8**)lbl_805FB964 + 0x18))(lbl_805FB964, buf, *(s32*)(buf + 0x10));
    }

    while (cur != NULL && cur != lbl_805FB95C && lbl_805FB9E4 < 9u) {
        if ((lbl_805FB99C > 0.15f + *(f32*)(cur + 0x8) && sent < 3) || *(f32*)(cur + 0x8) < 0.0f) {
            *(f32*)(cur + 0x8) = lbl_805FB99C;
            (*(MovieFnSend*)(*(u8**)lbl_805FB964 + 0x18))(lbl_805FB964, cur, *(s32*)(cur + 0x10));
            sent++;
        }
        u8* base = *(u8**)(lbl_804DD568 + 0x14);
        s32 tl = *(s32*)(lbl_804DD568 + 0xC);
        s32 idx = (s32)(cur - base) / 1024;
        if (idx == tl) {
            cur = NULL;
        } else {
            s32 cp = *(s32*)(lbl_804DD568 + 0x10);
            s32 next = (idx + 1) % cp;
            if (RingContains(*(s32*)(lbl_804DD568 + 0x8), tl, cp, next)) cur = base + (next << 10);
            else cur = NULL;
        }
    }

    lbl_805FB9D8 = lbl_805FB9D8 - 1;
    if (lbl_805FB9D8 == 0) fn_801F5E48(lbl_804FB5E0, 3);
}

// 0x801FCA38
// Starts the fade/zoom widget once: records the tick, clamps the centre (x, y) so the sprite
// (size read from the u16 pair at +4/+6 of lbl_805FBA78) stays inside the 82.4% x 78.8% safe area,
// and fills the 3x4 transform at lbl_804FDA60 and the four corners at lbl_804FDA40.
// Returns 1 when started, 0 when already running or not ready.
s32 fn_801FCA38(f32 x, f32 y) {
    if (lbl_805FBA70 != 0 || lbl_805FBA78 == NULL || lbl_805F98B8[0] == 0) return 0;
    lbl_805FBA70 = 1;
    lbl_805FBA74 = OSGetTick();
    lbl_805FBA7C = 0;

    u16 sizeW = *(u16*)(lbl_805FBA78 + 0x4);
    u16 sizeH = *(u16*)(lbl_805FBA78 + 0x6);
    f32 halfW = (f32)(sizeW >> 1);
    f32 halfH = (f32)(sizeH >> 1);
    f32 screenW = (f32)gScreenWidth;
    f32 screenH = (f32)gScreenHeight;
    f32 marginX = (screenW - 0.824f * screenW) * 0.5f;
    f32 marginY = (screenH - 0.788f * screenH) * 0.5f;

    f32 minX = marginX + halfW;
    if (x < minX) {
        x = minX;
    } else {
        f32 maxX = (screenW - marginX) - halfW;
        if (x > maxX) x = maxX;
    }
    f32 minY = marginY + halfH;
    if (y < minY) {
        y = minY;
    } else {
        f32 maxY = (screenH - marginY) - halfH;
        if (y > maxY) y = maxY;
    }

    f32 left = x - halfW;
    f32 top = y - halfH;
    f32 h16 = (f32)sizeH;
    f32 w16 = (f32)sizeW;
    f32 zero = 0.0f;
    f32 one = 1.0f;
    f32 z8 = zero * one;
    f32 z4 = zero * h16;
    f32 zC = zero * w16;
    f32 right = x + halfW;
    f32 bottom = y + halfH;

    lbl_804FDA60[0] = one * w16;
    lbl_804FDA60[1] = z4;
    lbl_804FDA60[2] = z8;
    lbl_804FDA60[3] = zC;
    lbl_804FDA60[4] = one * h16;
    lbl_804FDA60[5] = z8;
    lbl_804FDA60[6] = zC;
    lbl_804FDA60[7] = z4;
    lbl_804FDA60[8] = one * one;
    lbl_804FDA60[9] = zC + left;
    lbl_804FDA60[10] = z4 + top;
    lbl_804FDA60[11] = z8 + zero;

    lbl_804FDA40[0] = left;
    lbl_804FDA40[1] = top;
    lbl_804FDA40[2] = right;
    lbl_804FDA40[3] = top;
    lbl_804FDA40[4] = right;
    lbl_804FDA40[5] = bottom;
    lbl_804FDA40[6] = left;
    lbl_804FDA40[7] = bottom;
    return 1;
}

// 0x802262E0
// Rolls the reward of a finished challenge: `table` is the challenge's row (0x28 bytes at
// db+0x7468). When a monster is given and the save has a pending unlock reward whose id (word at
// +0x10 of its 0x3C-byte row at db+0x8D58) equals the save's word at +0x130, that reward is
// given instead (and removed from the pending list). Otherwise rolls a random number of rewards
// (s16 range at +4..+6): kind 0 material (needs the monster), else kind 1/2 via fn_802268BC.
// Returns the MSP given by the unlock reward (0 otherwise).
s32 SaveRollChallengeReward(u8* s, s32 table, u8* monster) {
    s32 result = 0;
    LabQuestFinished(gLab, table);
    if (table == -1) return 0;

    u8 rec[0x28];
    memcpy(rec, *(u8**)(gGameDb->data + 0x7468) + table * 0x28, 0x28);

    if (monster != NULL) {
        s32 found = 0;
        for (s32 i = 0; i < (s32)s[0x124]; i++) {
            u8 row[0x3C];
            memcpy(row, *(u8**)(gGameDb->data + 0x8D58) + *(s32*)(s + 0x6BC + i * 4) * 0x3C, 0x3C);
            u32 id = *(u32*)(row + 0x10);
            if (id != 0 && id == *(u32*)(s + 0x130)) {
                result = SaveUnlockReward(s, row, 0);
                if ((u32)i < s[0x124]) {
                    // Remove from the pending list by moving the last entry into the hole.
                    *(s32*)(s + 0x6BC + i * 4) = *(s32*)(s + 0x6BC + (s[0x124] - 1) * 4);
                    *(s32*)(s + 0x6BC + (s[0x124] - 1) * 4) = -1;
                    s[0x124] = (u8)(s[0x124] - 1);
                }
                found = 1;
                break;
            }
        }
        if (found) return result;
    }

    if (table > 0x3A) return result;

    s32 count = *(s16*)(rec + 0x6);
    s32 minCount = *(s16*)(rec + 0x4);
    if (count - minCount > 0) count = minCount + RandomUpTo(count - minCount);

    s32 w0 = *(s16*)(rec + 0x8);
    s32 w01 = w0 + *(s16*)(rec + 0xA);
    s32 total = *(s16*)(rec + 0xC) + w01;
    for (s32 n = 0; n < count; n++) {
        s32 r = RandomUpTo(total - 1);
        if (r < w0) fn_8022667C(s, rec, (void**)monster);
        else fn_802268BC(s, rec);   // both the r < w01 and the remaining branch call this
    }
    return result;
}

}  // extern "C"
