// Missing functions batch 113: lab-book slider setters, candidate scoring, combat action
// selection and results screen, lightning between locators, conversation outro, per-frame
// game update and controller polling, save file open.
#include <string.h>

#include "types.h"
#include "engine/math.h"
#include "engine/rend.h"
#include "game/part.h"

extern "C" {
#include "game/script_callbacks.h"
}

typedef void (*VFn0)(u8*);
typedef void (*VFnI)(u8*, s32);
typedef void (*VFnF)(u8*, f32);
typedef u8* (*VFnP)(u8*);
typedef u8* (*HashMethod)(u8* self, u32 hash);

extern "C" {
void ChoiceInit(u8* c);                                          // 0x8020F36C
void* ChoiceDestroy(void* c, s32 del);                           // 0x8020F394
void CombatHudRefreshActions(void);                              // 0x800476D8
u8** FighterGetSlot(u8* f, s32 i);                               // 0x8020E794
BOOL IsOnlineBattle(void);                                       // 0x80220A24
s32 MonsterGetName(u8* m, char* out, s32 size);                  // 0x80220394
void* ScreenSetFindElement(void* set, u32 hash);                 // 0x80198F18
void* ScreenSetFindWidget(void* set, u32 hash);                  // 0x80198E34
void ScreenSetsActivate(void* sets, u32 hash);                   // 0x8018CD8C
void* ScreenSetsCurrent(void* sets);                             // 0x8018D1E4
u32 StringHash(const char* s);                                   // 0x801EB704
s32 SysSprintf(char* buf, const char* fmt, ...);                 // 0x80142EFC
void TargetListDecode(void* t, const u32* mask, s32 side);       // 0x80216240
void* TargetListDestroy(void* t, s32 del);                       // 0x80216200
void TargetListInit(void* t);                                    // 0x802161DC
void TextureRelease(void* tex);                                  // 0x80184028
void UISetNumber(void* el, s32 n);                               // 0x8019F2A8
void UISetText(void* el, const char* text, s32 a);               // 0x8019EFC8
s32 WorldGetPhase(void* world);                                  // 0x800D83A8

s32  fn_8000AF80(u8* ctx, s32 a, s32 b, s32 c);                 // 0x8000AF80
void fn_8000B880(u8* ctx, s32 state);                            // 0x8000B880
u32 fn_8000B888(u8* ctx);                                        // 0x8000B888
u32 fn_8000B894(u8* ctx);                                        // 0x8000B894
u32 fn_8000B8B8(u8* ctx);                                        // 0x8000B8B8
s32 fn_8000B8CC(u8* ctx);                                        // 0x8000B8CC
void fn_8000B948(u8* ctx);                                       // 0x8000B948
void fn_8000B97C(u8* ctx);                                       // 0x8000B97C
void fn_8000B98C(u8* ctx);                                       // 0x8000B98C
void fn_8000BD88(u8* ctx);                                       // 0x8000BD88
void fn_8000C17C(u8* ctx);                                       // 0x8000C17C
void fn_8000D3AC(void);                                          // 0x8000D3AC
void fn_800493A8(s32 on);                                        // 0x800493A8
u8* fn_8004EDD0(u8* obj, s32 flag, s32 a2);                      // 0x8004EDD0
u8* fn_800E5684(u8* p);                                          // 0x800E5684
void* fn_800E60BC(void);                                         // 0x800E60BC
void fn_8012649C(void);                                          // 0x8012649C
void fn_80126808(void);                                          // 0x80126808
void fn_8013F160(void);                                          // 0x8013F160
void fn_80140D3C(void);                                          // 0x80140D3C
s32 fn_801481FC(s32 chan);                                       // 0x801481FC
void fn_8014DE68(void);                                          // 0x8014DE68
s32 fn_8015BA30(void* p);                                        // 0x8015BA30
s32 fn_8018CA38(u8* sets, f32 dt);                            // 0x8018CA38
void fn_8018DBB8(void* sets, s32 a);                             // 0x8018DBB8
s32 fn_80198A8C(void* set, void* widget);                       // 0x80198A8C
BOOL fn_80198C34(void* set, void* w);                            // 0x80198C34
ScriptValue fn_801A1208(u8* el);                                 // 0x801A1208 (image element -> texture)
s32 fn_800ED944(u8* p, f32 dt);                                          // 0x800ED944 (lightning callback)
void fn_801B55F4(u8* obj, u8* out, s32 idx);                     // 0x801B55F4
void fn_801B9C94(u8* mgr);                                       // 0x801B9C94
void fn_801B9CA8(u8* mgr);                                       // 0x801B9CA8
void fn_801B9CBC(void* mgr, BOOL (*cb)(void**));                 // 0x801B9CBC
void fn_801B9DD0(u8* mgr);                                       // 0x801B9DD0
u8* fn_801BC1C0(u8* node, s32 id);                               // 0x801BC1C0
void fn_801E4918(u8* obj, s32 a, f32 t);                         // 0x801E4918
void fn_801E5C3C(u8* obj, s32 a, f32 t);                         // 0x801E5C3C
void fn_801E8D74(void* out, void* list, void* owner, void* anim);  // 0x801E8D74
void fn_801F0564(void);                                          // 0x801F0564
void fn_801F357C(void);                                          // 0x801F357C
void fn_801F3BB8(void);                                          // 0x801F3BB8
void fn_801F401C(void);                                          // 0x801F401C
s32 fn_801F525C(void);                                           // 0x801F525C
void fn_80204978(const char* s);                                 // 0x80204978
void fn_80206B6C(s32 code, const u8* msg);                       // 0x80206B6C
void fn_8020B5E8(u8* p, f32 a, f32 b);                           // 0x8020B5E8
s32 fn_80212E10(u8* battle);                                     // 0x80212E10
s32 fn_80265C18(const char* path, s32 mode, s32 a);              // 0x80265C18
s32 fn_8026631C(const char* path, s32 mode, s32 a);              // 0x8026631C
s32 fn_80266BA4(const char* name, u8* handle, s32 flags);        // 0x80266BA4
s32 fn_80267A54(u32* buf);                                       // 0x80267A54
s32 fn_80267B14(u32* buf);                                       // 0x80267B14
void WPADDisconnect(s32 chan, s32 b);                               // 0x802727E0
s32 WPADProbe(s32 chan, u32* out);                             // 0x802728A4
s32 fn_80272B64(s32 chan, u8* out);                              // 0x80272B64
char* fn_80338A9C(char* s, const char* delim);                   // 0x80338A9C (strtok)

// lab books: ingredient / part / recipe / statistics / badge sections
void fn_801195F0(u8* sub, s32 a, s32 b);                         // 0x801195F0
void fn_801195F8(u8* sub, s32 a);                                // 0x801195F8
void fn_8011BA6C(u8* sub, s32 a, s32 b);                         // 0x8011BA6C
void fn_8011BA74(u8* sub, s32 a);                                // 0x8011BA74
void fn_8011D028(u8* sub, s32 a, s32 b);                         // 0x8011D028
void fn_8011D030(u8* sub, s32 a);                                // 0x8011D030
void fn_8011E9A8(u8* sub, s32 a, s32 b);                         // 0x8011E9A8
void fn_8011E9B0(u8* sub, s32 a);                                // 0x8011E9B0
void fn_80120978(u8* sub, s32 a, s32 b);                         // 0x80120978
void fn_80120984(u8* sub, s32 a);                                // 0x80120984

extern u8 gGame[];                                               // 0x803B53A0
extern u8 gIdPool[];                                             // global id pool
extern u8 gScriptCtx[];                                          // 0x803C4298
extern u8 gWorldState[];                                         // 0x803E6D00
extern "C++" u8 sSirHeader[0x1000];                                    // 0x803C2A20
extern u8 lbl_805FB248[];                                        // 0x805FB248 (lab books section table pointer)
extern u8 lbl_8048A8B4[];                                        // 0x8048A8B4
extern u8 lbl_80386C10[];                                        // 0x80386C10 (save error messages)
extern u8 lbl_80527140[];                                        // 0x80527140 (save buffer)
extern u8 lbl_803C529C[];                                        // 0x803C529C (UI state block)
extern u8 lbl_805FB0AA[];                                        // 0x805FB0AA
extern u8 lbl_805FB099[];                                        // 0x805FB099
extern u32 lbl_805FB0A0;                                         // 0x805FB0A0 (last disabled HUD input hash)
extern u32 lbl_805FB1EC;                                         // 0x805FB1EC (StringHash cache)
extern u8 lbl_805FB1F0;                                          // 0x805FB1F0 (cache valid flag)
extern s32 lbl_805FBAB0;                                         // 0x805FBAB0
extern f32 lbl_805F95E0;                                         // 0x805F95E0
extern f32 lbl_805FC698;                                         // 0x805FC698
extern f32 lbl_805FB920;                                         // 0x805FB920
extern f32 lbl_805FB924;                                         // 0x805FB924
extern f32 lbl_805FC610;                                         // 0x805FC610
extern f32 lbl_805FC614;                                         // 0x805FC614
extern f32 lbl_805FC630;                                         // 0x805FC630
extern f32 lbl_805FC634;                                         // 0x805FC634
extern f32 lbl_805FC658;                                         // 0x805FC658
extern f32 lbl_805FC660;                                         // 0x805FC660
extern f32 lbl_805FEB0C;                                         // 0x805FEB0C (0.0f)
extern f32 lbl_805FEB74;                                         // 0x805FEB74
extern const char lbl_8036045C[];                                // 0x8036045C (effect name string)
extern const char lbl_80353758[];                                // 0x80353758
extern const char lbl_80358AF0[];                                // 0x80358AF0
extern const char lbl_805F81E0[];                                // 0x805F81E0
extern const char lbl_805F81E8[];                                // 0x805F81E8
extern const char lbl_805F9908[];                                // 0x805F9908 (path separators)
extern const char lbl_805F990C[];                                // 0x805F990C (dir path format)
}

extern "C++" void UiUpdate(f32 dt);                              // 0x8000C838
extern "C++" BOOL fn_8000F25C(void** screens);                   // 0x8000F25C
extern "C++" void* gEventMgr;                                    // 0x805FB6C8
extern "C++" void* gScreenSets;                                  // 0x805FB654
extern "C++" u8* gBattle;                                        // 0x805FBBB8
extern "C++" u8* gSaveSystem;                                    // 0x805FBBE0
extern "C++" s32 gSelectedPart;                                  // 0x805F8650
extern "C++" s32 gPartSlotState[10];                             // 0x803C52E0
extern "C++" u8* fn_801B55A8(u8* ent);                           // 0x801B55A8 (C++ linkage; returns the locator count as an integer value)
extern "C++" f32 gFrameTime;                                     // 0x805FB918
extern "C++" s32 gScreenHeight;                                  // 0x805F938C
extern "C++" s32 gViewportIndex;                                 // 0x805FB4BC
extern "C++" s32 gViewportCount;                                 // 0x805FB4C0
extern "C++" s32 gFrameBudget;                                   // 0x805FB4C8
extern "C++" u8 sRendDLs[];                                      // 0x80401108 (display-list records, 0x94 bytes each)

// StringHash constants (CRC-32 of the upper-cased name, see tools/hashes.py).
static const u32 kHashIngredients = 0xC66383A7u;  // "ingredients"
static const u32 kHashParts       = 0x9E14654Eu;  // "parts"
static const u32 kHashRecipes     = 0x9C2C1CAFu;  // "recipes"
static const u32 kHashStatistics  = 0xE94C961Du;  // "Statistics"
static const u32 kHashBadges      = 0x880E941Cu;  // "badges"
static const u32 kHashCombatHud   = 0x1B66AC94u;  // "UI_MSA_Combat_HUD"
static const u32 kHashTitle       = 0xDC62BADBu;  // "Title"
static const u32 kHashWins        = 0x944D3C8Du;  // "Wins"
static const u32 kHashLosses      = 0xDBD52F87u;  // "Losses"
static const u32 kHashCancelRing  = 0x1B6732BBu;  // widget polled by the controller update

// Calls the virtual slot at byte offset `off` of obj with a hash argument; returns r3.
static u8* CallVirt(u8* obj, s32 off, u32 hash) {
    HashMethod fn = *(HashMethod*)(*(u8**)obj + off);
    return fn(obj, hash);
}

// sub-object (+off of the lab books table)->vfunc[0x28](hash), or NULL when it is missing.
static u8* LabSubQuery(s32 off, u32 hash) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return 0;
    u8* sub = *(u8**)(top + off);
    if (sub == 0) return 0;
    return CallVirt(sub, 0x28, hash);
}

// True when the active lab books section is `screen` and its vfunc[0x24](hash) agrees.
static bool LabScreenIs(s32 screen, u32 hash) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return false;
    if (*(s32*)top != screen) return false;
    return CallVirt(*(u8**)(top + screen * 4 + 4), 0x24, hash) != 0;
}

// Score of a candidate entity: its squared distance to the world focus point plus its squared
// distance to a point ahead of the camera (camera position + camera Y axis * lbl_805FC610),
// both weighted by lbl_805FC614.
static f32 CandidateScore(u8* ent) {
    const f32* p = *(const f32**)(ent + 4);
    f32 px = p[0], py = p[1], pz = p[2];
    u8* cam = *(u8**)(*(u8**)(gScriptCtx + 4) + 4);
    Vec3 axis = QuatGetAxisY((const Quat*)(cam + 0xC));
    u8* w = *(u8**)gWorldState;
    w = *(u8**)(w + 8);
    const f32* c = *(const f32**)(w + 4);
    f32 k = lbl_805FC610;
    f32 kk = lbl_805FC614;
    f32 ax = axis.x * k, ay = axis.y * k, az = axis.z * k;
    f32 fx = *(f32*)(cam + 0) + ax;
    f32 fy = *(f32*)(cam + 4) + ay;
    f32 fz = *(f32*)(cam + 8) + az;
    f32 d1x = px - c[0], d1y = py - c[1], d1z = pz - c[2];
    f32 d2x = px - fx, d2y = py - fy, d2z = pz - fz;
    f32 s1 = (d1x * d1x + d1y * d1y) + d1z * d1z;
    f32 s2 = (d2x * d2x + d2y * d2y) + d2z * d2z;
    return kk * s2 + kk * s1;
}

extern "C" {

// 0x80006F48
// Camera target scoring, "activate" pass: skips entities that are not selectable; otherwise
// keeps the lowest-priority candidate (+0x24 of the record), ties broken by the larger score.
// (The asm has two identical score paths, selected by the byte at 0x805FB948.)
s32 ScoreActivateCandidate(u8* ent, s32 unused, u8* rec) {
    u8* def = *(u8**)(ent + 4);
    if ((*(u32*)(def + 0x20) & 3) == 0) return 1;
    u8* extra = *(u8**)(ent + 0x9C);
    if (extra == 0) return 1;
    *(f32*)rec = *(f32*)rec - *(f32*)(extra + 0xC0);
    def = *(u8**)(ent + 4);
    if (*(u32*)(def + 0x20) & 0x80000) return 1;
    if (*(u32*)(ent + 0x864) > *(u32*)(rec + 0x24) || *(s32*)(ent + 0x78C) == 1) return 1;
    f32 score = CandidateScore(ent);
    if (*(u32*)(rec + 0x24) > *(u32*)(ent + 0x864)) {
        *(u8**)(rec + 0xC) = ent;
        *(f32*)(rec + 0x10) = score;
        *(u32*)(rec + 0x24) = *(u32*)(ent + 0x864);
    } else if (score > *(f32*)(rec + 0x10)) {
        *(u8**)(rec + 0xC) = ent;
        *(f32*)(rec + 0x10) = score;
    }
    return 1;
}

// 0x80007464
// Camera target scoring, "cull" pass: keeps the highest-priority candidate (+0x20 of the
// record), ties broken by the smaller score.
s32 ScoreCullCandidate(u8* ent, s32 unused, u8* rec) {
    if (*(u32*)(ent + 0x864) < *(u32*)(rec + 0x20)) return 1;
    u8* def = *(u8**)(ent + 4);
    if (*(u32*)(def + 0x20) & 0x80) return 1;
    f32 score = CandidateScore(ent);
    if (*(u32*)(ent + 0x864) > *(u32*)(rec + 0x20)) {
        *(u8**)(rec + 4) = ent;
        *(f32*)(rec + 8) = score;
        *(u32*)(rec + 0x20) = *(u32*)(ent + 0x864);
        *(u32*)(rec + 0x24) = *(u32*)(ent + 0x864);
    } else if (score < *(f32*)(rec + 8)) {
        *(u8**)(rec + 4) = ent;
        *(f32*)(rec + 8) = score;
    }
    return 1;
}

// 0x8000B9CC
// Per-frame controller poll: tracks the controller connect state on channel 0, counts down the
// reconnect timer, and drives the "reconnect controller" prompt widget while the script
// context is in state 5 or 7.
void fn_8000B9CC(u8* ctx, f32 dt) {
    for (s32 i = 1; i <= 3; i++) {
        if (fn_801481FC(i) != 0) WPADDisconnect(i, 0);
    }
    if (fn_801481FC(0) == 0) {
        u32 st = 0;
        if (WPADProbe(0, &st) == -1 && *(s32*)(ctx + 0x158) != 1) {
            *(s32*)(ctx + 0x158) = 1;
            *(f32*)(ctx + 0x164) = lbl_805FC658;
        }
    } else {
        u32 st = 0;
        if (WPADProbe(0, &st) == 0 && st != 0xFF) {
            if (st != 1 && *(f32*)(ctx + 0x164) <= lbl_805FC630) {
                if (*(s32*)(ctx + 0x158) != 2) *(s32*)(ctx + 0x158) = 2;
            } else if (*(f32*)(ctx + 0x164) > lbl_805FC630) {
                *(f32*)(ctx + 0x164) = *(f32*)(ctx + 0x164) - dt;
            } else if (*(s32*)(ctx + 0x158) != 0) {
                u32 st2 = 0;
                *(s32*)(ctx + 0x158) = 0;
                if (WPADProbe(0, &st2) == 0) {
                    u8 info[0x1C];
                    if (fn_80272B64(0, info) == 0 && *(s32*)(info + 0xC) != 0) {
                        *(s32*)(ctx + 0x154) = 1;
                    }
                }
            }
        }
    }
    if (*(s32*)(ctx + 0x18) != 5 && *(s32*)(ctx + 0x18) != 7) return;

    s32 on = 0;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set != 0) {
        void* w = ScreenSetFindWidget(set, kHashCancelRing);
        if (w != 0 && fn_80198C34(set, w) != 0) on = 1;
    }
    f32 t = *(f32*)(ctx + 0x160);
    if (t >= lbl_805FC630 && on != 0) {
        *(f32*)(ctx + 0x160) = t + gFrameTime;
    } else if (on == 0 && *(s32*)(ctx + 0x18) != 5 && (*(u32*)(ctx + 0x148) & 8) != 0) {
        *(u32*)(ctx + 0x148) = ((*(u32*)(ctx + 0x148) & ~8u) | 2u) & ~1u;
        GameFlowSignal(0x0FCE227A, 0, 0);
    }
    if (*(u8*)(ctx + 0x1C) != 0x2A) {
        if (*(s32*)(ctx + 0x158) != 0 || *(s32*)(ctx + 0x154) != 0) {
            fn_8000C17C(ctx);
            fn_8000BD88(ctx);
        }
    }
    s32 pending = *(s32*)(ctx + 0x158) != 0;
    s32 hold = 0;
    if (*(s32*)(ctx + 0x154) != 0) {
        f32 t2 = *(f32*)(ctx + 0x160);
        if (t2 >= lbl_805FC630 && t2 < lbl_805FC660) hold = 1;
    }
    if (on != 0 && pending == 0 && hold == 0) {
        void* cur = ScreenSetsCurrent(gScreenSets);
        if (cur != 0) {
            void* w = ScreenSetFindWidget(cur, kHashCancelRing);
            if (w != 0 && fn_80198C34(cur, w) != 0) fn_80198A8C(cur, w);
            *(s32*)(ctx + 0x15C) = 0;
            fn_8018DBB8(gScreenSets, 1);
        }
        if (*(f32*)(ctx + 0x160) >= lbl_805FC630) {
            *(s32*)(ctx + 0x154) = 0;
            *(f32*)(ctx + 0x160) = lbl_805FC634;
        }
        if (*(s32*)(ctx + 0x18) != 5) {
            *(u32*)(ctx + 0x148) = ((*(u32*)(ctx + 0x148) & ~8u) | 2u) & ~1u;
            GameFlowSignal(0x0FCE227A, 0, 0);
        }
    }
}

// 0x8000F298
// Per-frame game update: ticks the effect pools, runs the script context state machine
// (pause / resume / game-event hand-off), then the UI, input and event manager updates and the
// display-list budget check.
void fn_8000F298(void) {
    fn_801F0564();
    if (*(u32*)(gScriptCtx + 0x3C) & 2) lbl_805F95E0 = lbl_805FC698;
    if (lbl_805FBAB0 != 0) {
        char buf[0x400];
        SysSprintf(buf, lbl_80353758, lbl_805F81E0, lbl_805F81E8);
        fn_80204978(buf);
    }
    fn_801E4918(lbl_8048A8B4, 0, gFrameTime);
    fn_801E5C3C(gIdPool, 0, gFrameTime);
    fn_8013F160();
    fn_80140D3C();
    u8* world = *(u8**)(gGame + 0xD614);
    u8* arg = 0;
    if (world != 0 && *(u8**)(world + 4) != 0) arg = *(u8**)(world + 4);
    fn_8020B5E8(arg, lbl_805FB924, lbl_805FB920);
    fn_801B9C94((u8*)gEventMgr);

    if (fn_8000B888(gScriptCtx) != 0) {
        u8* item = 0;
        u8* node = *(u8**)(sSirHeader + 0xD98);
        if (node != 0) {
            s32 idx = *(s32*)(node + 4);
            u8* cur = idx > -1 ? *(u8**)*(u8**)node + idx * 0x30 : 0;
            if (cur != 0 && fn_8000B8CC(gScriptCtx) != 0) {
                node = *(u8**)(sSirHeader + 0xD98);
                idx = *(s32*)(node + 4);
                cur = idx > -1 ? *(u8**)*(u8**)node + idx * 0x30 : 0;
                item = fn_801BC1C0(cur, fn_8000B8CC(gScriptCtx));
            }
        }
        if (fn_8000B8CC(gScriptCtx) == 0x20CE505D || (item != 0 && CheckNoGameEventsActive() != 0)) {
            if (*(s32*)(gScriptCtx + 0x18) == 5) {
                fn_801B9CBC(gEventMgr, fn_8000F25C);
                fn_8000B880(gScriptCtx, 7);
                fn_8000AF80(gScriptCtx, 1, 0, 0);
            }
            GameFlowSignal(fn_8000B8CC(gScriptCtx), 0, 0);
            fn_8000B98C(gScriptCtx);
            fn_80126808();
        }
        fn_8000B948(gScriptCtx);
    } else if (fn_8000B894(gScriptCtx) != 0) {
        if (*(s32*)(gScriptCtx + 0x18) == 7 && fn_8000B8B8(gScriptCtx) != 0) {
            fn_8000B880(gScriptCtx, 5);
            fn_8000AF80(gScriptCtx, 0, 0, 0);
            fn_801B9DD0((u8*)gEventMgr);
            fn_80126808();
        }
        fn_8000B97C(gScriptCtx);
    }
    fn_8000D3AC();
    if (fn_801F525C() == 0) fn_801F357C();
    fn_8018CA38((u8*)gScreenSets, gFrameTime);
    UiUpdate(gFrameTime);
    fn_801F3BB8();
    fn_801F401C();
    fn_801B9CA8((u8*)gEventMgr);
    if (gViewportCount != 0) {
        if (fn_8015BA30(sRendDLs + gViewportIndex * 0x94) + 0x10C0 > gFrameBudget) fn_8014DE68();
    }
}

// 0x80046808
// Combat HUD: the player picked action slot a0 (0 = first, else second action) of the selected
// part. Checks the action can target a limb, flags the turn as chosen and signals the HUD;
// the "special" action type is blocked until the tutorial flag is set.
void SetActionSelected(s32 a0) {
    PartDef def;
    u8** parts = *(u8***)(gBattle + 0xA00);
    PartDbGetDef(&def, gGameDb, **(s32**)parts[gSelectedPart]);
    s32 actionId = (a0 == 0) ? def.action1 : def.action2;
    u8* act = *(u8**)(gGameDb->data + 0x430) + actionId * 0x14;
    u32 key = *(u32*)act;
    s32 type = *(s32*)(act + 4);
    u32 targets = *(u32*)(act + 8);

    if (lbl_805FB0AA[0] != 0) {
        struct { s32 limbs[5]; s32 count; } list;
        TargetListInit(&list);
        u32 mask = targets;
        TargetListDecode(&list, &mask, gSelectedPart);
        s32 found = 0;
        for (s32 i = 0; i < list.count; i++) {
            if (list.limbs[i] == 3) { found = 1; break; }
        }
        if (found == 0) {
            SendNISSignal(0x42A334DC);
            TargetListDestroy(&list, -1);
            return;
        }
        lbl_805FB0AA[0] = 0;
        SendNISSignal(0xA9E9D05B);
        *(s32*)(gBattle + 0xAD4) = 1;
        **(s32**)FighterGetSlot(*(u8**)(gBattle + 0xA04), 3) = 1;
        u8 choice[0xC];
        ChoiceInit(choice);
        *(s32*)(choice + 8) = 4;
        choice[4] = 0;
        *(s32*)choice = 0x4A;
        u8* fighter = *(u8**)(gBattle + 0xA04);
        *(s32*)(fighter + 0x38) = *(s32*)choice;
        *(u8*)(fighter + 0x3C) = choice[4];
        *(s32*)(fighter + 0x40) = *(s32*)(choice + 8);
        ChoiceDestroy(choice, -1);
        TargetListDestroy(&list, -1);
    }

    s32 stat = *(s32*)(*(u8**)(gGameDb->data + 0x8) + type * 0x24 + 8);
    if (stat == 5 && *(s32*)(gBattle + 0xAD0) != 4 && CheckGameProgressBitFlag(0xF) == 0 && IsMultiPlayer() == 0) {
        CombatHUDDisableInput(0xADBF3357);
        SendNISSignal(0x6F0F0B93);
        return;
    }
    lbl_803C529C[4] = 0;
    *(s32*)(lbl_803C529C + 8) = gSelectedPart;
    *(u32*)lbl_803C529C = key;
    lbl_805FB099[0] = 1;
    SendNISSignal(0x200523BE);
}

// 0x800479D8
// Sets up the combat results screen: clears the part slot states, shows the result banner
// (text and event depend on the battle outcome), the monster name and win/loss counters, and
// rescales the vertical position of the banner image to the current screen height.
void SetUpCombatResultsScreen(void) {
    for (s32 i = 0; i < 10; i++) gPartSlotState[i] = -1;
    CombatHudRefreshActions();
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == 0) return;
    if (*(u32*)set != kHashCombatHud) return;

    ScreenSetsActivate(gScreenSets, 0x7A1060A7);
    u8* el = (u8*)ScreenSetFindElement(set, 0x7A1060A7);
    if (el != 0) (*(VFnI*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
    fn_800493A8(1);
    CombatHUDDisableInput(lbl_805FB0A0);
    if (fn_80212E10(gBattle) != 0) {
        UIText_SetString_TextID(0xF16D9879, 0x685);
        UIEvent_SendToScreen(0x89200344, 0xB93CA1BD);
    } else {
        UIText_SetString_TextID(0xF16D9879, 0x58C);
        UIEvent_SendToScreen(0xE53BC1BD, 0xB93CA1BD);
    }

    char name[0x100];
    memset(name, 0, sizeof(name));
    u8* title = (u8*)ScreenSetFindElement(set, kHashTitle);
    if (title != 0) {
        u8* mon = *(u8**)(*(u8**)(gBattle + 0xA00) + 0x1C);
        if (mon != 0) mon -= 0xC48;
        if (MonsterGetName(mon + 0x898, name, 0x100) <= 0) SysSprintf(name, lbl_80358AF0, 0);
        UISetText(title, name, 1);
        UIEvent_SendToScreen(0xD473317D, 0xB93CA1BD);
    }
    u8* wins = (u8*)ScreenSetFindElement(set, kHashWins);
    if (wins != 0) {
        s32 n = IsOnlineBattle() ? *(s32*)(gSaveSystem + 0xA28) : *(s32*)(gSaveSystem + 0x8F0);
        UISetNumber(wins, n);
        (*(VFnI*)(*(u8**)(wins + 0x138) + 0x2C))(wins, 2);
    }
    u8* losses = (u8*)ScreenSetFindElement(set, kHashLosses);
    if (losses != 0) {
        s32 n = IsOnlineBattle() ? *(s32*)(gSaveSystem + 0xA2C) : *(s32*)(gSaveSystem + 0x8F4);
        UISetNumber(losses, n);
        (*(VFnI*)(*(u8**)(losses + 0x138) + 0x2C))(losses, 2);
    }
    u8* banner = (u8*)ScreenSetFindElement(set, 0x1B49766F);
    if (banner != 0) {
        u8* tex = (u8*)(intptr_t)fn_801A1208(banner);
        f32 texH = (f32)*(u16*)(tex + 6);
        f32 screenH = (f32)gScreenHeight;
        f32 videoH = (f32)VideoGetHeight(&gVideo);
        *(f32*)(banner + 0x78) = (texH * screenH) / videoH;
        (*(VFnI*)(*(u8**)(banner + 0x138) + 0x2C))(banner, 0x18);
    }
}

// 0x800EE9D0
// Spawns a lightning effect between two locators of an object (each given by locator index or
// by a model-node index); a0 = object, a1/a2 = locator records, a3 = duration (< 0: flag),
// a4 = effect name hash (0 = default), a5 = extra argument.
void SpawnLightningBetweenChildren(void* a0, u32 /*0x30*/ a1, u32 /*0x30*/ a2, f32 a3, u32 a4, u32 a5) {
    u8* obj = (u8*)a0;
    u8* recA = (u8*)(uintptr_t)a1;
    u8* recB = (u8*)(uintptr_t)a2;
    u32 hash = a4;
    if (obj == NULL) return;

    f32 locA[12];
    f32 locB[12];
    u16 flags = *(u16*)recA;
    s32 idx = (flags & 1) ? *(u8*)(recA + 2) : -1;
    s32 have;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(obj)) {
            fn_801B55F4(obj, (u8*)locA, idx);
            have = 1;
        } else {
            have = 0;
        }
    } else {
        s32 idx2 = (flags & 2) ? *(u8*)(recA + 2) : -1;
        if (idx2 != -1) {
            have = 1;
            u8* src = *(u8**)(obj + 0x98) + idx2 * 0xC8;
            for (s32 i = 0; i < 12; i++) locA[i] = *(f32*)(src + 0x38 + i * 4);
        } else {
            have = 0;
        }
    }
    if (have == 0) return;

    flags = *(u16*)recB;
    idx = (flags & 1) ? *(u8*)(recB + 2) : -1;
    if (idx != -1) {
        if (idx < (s32)(intptr_t)fn_801B55A8(obj)) {
            fn_801B55F4(obj, (u8*)locB, idx);
            have = 1;
        } else {
            have = 0;
        }
    } else {
        s32 idx2 = (flags & 2) ? *(u8*)(recB + 2) : -1;
        if (idx2 != -1) {
            have = 1;
            u8* src = *(u8**)(obj + 0x98) + idx2 * 0xC8;
            for (s32 i = 0; i < 12; i++) locB[i] = *(f32*)(src + 0x38 + i * 4);
        } else {
            have = 0;
        }
    }
    if (have == 0) return;

    if (hash == 0) hash = 0xC20F6A80u;
    s32 idB = *(s32*)recB;
    s32 idA = *(s32*)recA;
    u32 entBuf[0x20];
    u8* ent = (u8*)entBuf;
    memset(ent, 0, 0x64);
    *(s32*)(ent + 0x64) = 0;
    *(s32*)(ent + 0x68) = 0;
    *(void**)(ent + 0x6C) = (void*)fn_800ED944;
    *(f32*)(ent + 0x70) = 0.0f;
    *(u16*)(ent + 0x78) = 0xFFFF;
    *(u16*)(ent + 0x7A) = 0xFFFF;
    *(u16*)(ent + 0x7C) = 0xFFFF;
    *(s32*)(ent + 0x74) = 0;
    ent[0x7E] = 0;
    ent[0x7F] = 0;
    *(u32*)(ent + 0x48) = hash;
    *(u32*)(ent + 0x4C) = a5;
    *(s32*)(ent + 0x0) = idA;
    *(s32*)(ent + 0x4) = idB;
    *(f32*)(ent + 0x10) = 0.0f;
    *(f32*)(ent + 0x14) = 0.0f;
    *(f32*)(ent + 0x18) = 1.0f;
    *(f32*)(ent + 0x1C) = 0.0f;
    *(f32*)(ent + 0x20) = 0.0f;
    *(f32*)(ent + 0x24) = 0.0f;
    *(f32*)(ent + 0x28) = (a3 >= 0.0f) ? a3 : 0.0f;
    *(f32*)(ent + 0x2C) = 0.0f;
    *(f32*)(ent + 0x30) = 0.0f;
    *(f32*)(ent + 0x34) = 0.0f;
    *(f32*)(ent + 0x38) = 0.0f;
    *(f32*)(ent + 0x3C) = 0.0f;
    *(s32*)(ent + 0x40) = 0;
    *(s32*)(ent + 0x44) = 0;
    *(s32*)(ent + 0x50) = (a3 < 0.0f ? 1 : 0) | 2;
    ent[0x8] = 0;
    ent[0x9] = 0;
    *(u16*)(ent + 0xA) = 0xFFFF;
    ent[0xC] = 0;
    ent[0xD] = 0;
    *(u16*)(ent + 0xE) = 0xFFFF;
    if ((s8)lbl_805FB1F0 == 0) {
        lbl_805FB1EC = StringHash(lbl_8036045C);
        lbl_805FB1F0 = 1;
    }
    *(u32*)(ent + 0x64) = lbl_805FB1EC;
    u32 tween[8];
    f32* tw = (f32*)tween;
    tw[1] = 0.0f;
    tw[2] = 0.0f;
    tw[3] = 1.0f;
    tw[4] = 0.0f;
    tw[5] = 0.0f;
    tw[6] = 0.0f;
    u8* wl = *(u8**)(*(u8**)gWorldState + 0x1C8);
    fn_801E8D74(tween, wl + *(u16*)(*(u8**)(obj + 4) + 0x32) * 0xC, obj, ent);
}

// 0x800F7EB0
// Wii conversation outro: depending on the outro kind (a0: 0/1, 2/4, 3, others none) fades the
// camera/world out (camera slots 0x10 and 0x30, or fn_8012649C), kind 3 also sends the
// conversation-end UI event and drops one reference to the portrait texture; finally the player
// is re-enabled (parts reset and the controller flag set).
void Wii_PlayConversationOutro(BOOL a0) {
    void* chk = fn_800E60BC();
    if (chk != 0) fn_800E5684((u8*)chk);

    s32 kind = a0;
    if (kind == 3) {
        UIEvent_SendToScreen(0xCB440EFA, 0xBEF77BAE);
    }
    if (kind == 0 || kind == 1) {
        s32 phase = WorldGetPhase(gGame + 0xD5F0);
        if (phase == 2) {
            *(s32*)(*(u8**)(gScriptCtx + 8) + 0x2C0) = *(s32*)(gGame + 0xD614);
            u8* cam = *(u8**)(gScriptCtx + 4);
            (*(VFn0*)(*(u8**)cam + 0x10))(cam);
            cam = *(u8**)(gScriptCtx + 4);
            (*(VFnF*)(*(u8**)cam + 0x30))(cam, lbl_805FEB0C);
        } else if (WorldGetPhase(gGame + 0xD5F0) == 1) {
            fn_8012649C();
        }
    } else if (kind == 2 || kind == 3 || kind == 4) {
        s32 phase = WorldGetPhase(gGame + 0xD5F0);
        if (phase == 2) {
            *(s32*)(*(u8**)(gScriptCtx + 8) + 0x2C0) = *(s32*)(gGame + 0xD614);
            u8* cam = *(u8**)(gScriptCtx + 4);
            (*(VFn0*)(*(u8**)cam + 0x10))(cam);
            cam = *(u8**)(gScriptCtx + 4);
            (*(VFnF*)(*(u8**)cam + 0x30))(cam, lbl_805FEB74);
        } else if (WorldGetPhase(gGame + 0xD5F0) == 1 || WorldGetPhase(gGame + 0xD5F0) == 7) {
            fn_8012649C();
        }
        if (kind == 3) {
            u8* tex = (u8*)(intptr_t)UIImage_GetTexture(0x5881687A);
            if (tex != 0) {
                s32 refs = *(s32*)(tex + 0x24) - 1;
                *(s32*)(tex + 0x24) = refs;
                if (refs == 0) TextureRelease(tex);
            }
        }
    }

    u8* player = *(u8**)(gGame + 0xD614);
    if (WorldGetPhase(gGame + 0xD5F0) == 2 && player != 0 && (*(u32*)(*(u8**)(player + 4) + 0x20) & 2) == 0) {
        u8* p = *(u8**)(gGame + 0xD614);
        if (*(u32*)(*(u8**)(p + 4) + 0x1C) & 0x2000) {
            for (s32 i = 0; i < 5; i++) {
                u8* slot = *(u8**)(p + 0xC50 + i * 4);
                if (slot != 0) fn_8004EDD0(slot, 1, 0);
            }
        }
        u8* sub = (*(VFnP*)(*(u8**)p + 0xC))(p);
        if (sub != 0) {
            sub = (*(VFnP*)(*(u8**)p + 0xC))(p);
            *(u32*)(sub + 8) |= 2;
        }
        u8* def = *(u8**)(p + 4);
        *(u32*)(def + 0x20) |= 2;
    }
}

// 0x80110FAC
// Lab books: sets the top and bottom of the slider on the active section (ingredients, parts,
// recipes, statistics or badges).
void Slider_SetTopAndBottom(u32 a0, u32 a1) {
    if (LabScreenIs(0, kHashIngredients)) {
        u8* sub = LabSubQuery(4, kHashIngredients);
        if (sub != 0) fn_801195F0(sub, (s32)a0, (s32)a1);
    } else if (LabScreenIs(0, kHashParts)) {
        u8* sub = LabSubQuery(4, kHashParts);
        if (sub != 0) fn_8011BA6C(sub, (s32)a0, (s32)a1);
    } else if (LabScreenIs(1, kHashRecipes)) {
        u8* sub = LabSubQuery(8, kHashRecipes);
        if (sub != 0) fn_80120978(sub, (s32)a0, (s32)a1);
    } else if (LabScreenIs(1, kHashStatistics)) {
        u8* sub = LabSubQuery(8, kHashStatistics);
        if (sub != 0) fn_8011E9A8(sub, (s32)a0, (s32)a1);
    } else if (LabScreenIs(1, kHashBadges)) {
        u8* sub = LabSubQuery(8, kHashBadges);
        if (sub != 0) fn_8011D028(sub, (s32)a0, (s32)a1);
    }
}

// 0x80111364
// Lab books: sets the slider image on the active section (same dispatch as above).
void Slider_SetImage(u32 a0) {
    if (LabScreenIs(0, kHashIngredients)) {
        u8* sub = LabSubQuery(4, kHashIngredients);
        if (sub != 0) fn_801195F8(sub, (s32)a0);
    } else if (LabScreenIs(0, kHashParts)) {
        u8* sub = LabSubQuery(4, kHashParts);
        if (sub != 0) fn_8011BA74(sub, (s32)a0);
    } else if (LabScreenIs(1, kHashRecipes)) {
        u8* sub = LabSubQuery(8, kHashRecipes);
        if (sub != 0) fn_80120984(sub, (s32)a0);
    } else if (LabScreenIs(1, kHashStatistics)) {
        u8* sub = LabSubQuery(8, kHashStatistics);
        if (sub != 0) fn_8011E9B0(sub, (s32)a0);
    } else if (LabScreenIs(1, kHashBadges)) {
        u8* sub = LabSubQuery(8, kHashBadges);
        if (sub != 0) fn_8011D030(sub, (s32)a0);
    }
}

// 0x80206474
// Opens a save file by path ("dir/dir/file"), creating the directories on the way and the file
// itself when the mode contains 'w'; returns 0 on success, -1 on failure (after logging the
// error). `mode` letters: 'r' = 1, 'w' = 2.
s32 SaveOpen(char* path, const char* mode) {
    u32 stat0[16];
    u32 stat1[16];
    char* tok[6];
    char dir[0x40];
    s32 rc;

    memset(stat0, 0, sizeof(stat0));
    rc = fn_80267B14(stat0);
    if (rc != 0) {
        fn_80206B6C(rc, lbl_80386C10 + 0x38);
    } else {
        rc = fn_80267A54(stat0);
        if (rc != 0) fn_80206B6C(rc, lbl_80386C10 + 0x48);
    }

    s32 n = 0;
    tok[0] = fn_80338A9C(path, lbl_805F9908);
    while (tok[n] != 0) {
        if (n >= 5) return -1;
        tok[n + 1] = fn_80338A9C(0, lbl_805F9908);
        n++;
    }
    if (n == 0) return -1;

    memset(dir, 0, sizeof(dir));
    for (s32 i = 0; i < n - 1; i++) {
        rc = fn_8026631C(tok[i], 0x30, 0);
        if (rc != 0 && rc != -6) {
            fn_80206B6C(rc, lbl_80386C10 + 0x58);
            return -1;
        }
        rc = fn_80267A54((u32*)tok[i]);
        if (rc != 0) {
            fn_80206B6C(rc, lbl_80386C10 + 0x48);
            return -1;
        }
        if (i == 0) {
            strcpy(dir, tok[i]);
        } else {
            SysSprintf(dir, lbl_805F990C, dir, tok[i]);
        }
    }
    if (n == 1) {
        dir[0] = 0;
    } else {
        path = tok[n - 1];
    }

    u8 flags = 0;
    if (mode != 0) {
        for (; *mode != 0; mode++) {
            if (*mode == 'r') flags |= 1;
            if (*mode == 'w') flags = (flags | 2) & 0xFF;
        }
    }
    if (flags & 2) {
        rc = fn_80265C18(path, 0x30, 0);
        if (rc != 0 && rc != -6) {
            fn_80206B6C(rc, lbl_80386C10 + 0x68);
            return -1;
        }
    }
    rc = fn_80266BA4(path, lbl_80527140, flags);
    if (rc != 0) {
        fn_80206B6C(rc, lbl_80386C10 + 0x74);
        return -1;
    }

    memset(stat1, 0, sizeof(stat1));
    rc = fn_80267B14(stat1);
    if (rc != 0) {
        fn_80206B6C(rc, lbl_80386C10 + 0x38);
    } else {
        rc = fn_80267A54(stat1);
        if (rc != 0) fn_80206B6C(rc, lbl_80386C10 + 0x48);
    }
    return 0;
}

}  // extern "C"
