// Lab books UI script callbacks: statistics page (0x8011xxxx).
#include "types.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
extern u8  lbl_805FB248[];                // 0x805FB248 (lab books section table pointer)

s32  fn_8011D71C(u8* p);                  // 0x8011D71C
s32  fn_8011D728(u8* p);                  // 0x8011D728
s32  fn_8011D79C(u8* p);                  // 0x8011D79C
s32  fn_8011D894(u8* p);                  // 0x8011D894
s32  fn_8011D930(u8* p);                  // 0x8011D930
s32  fn_8011D97C(u8* p);                  // 0x8011D97C
s32  fn_8011D988(u8* p);                  // 0x8011D988
s32  fn_8011D994(u8* p);                  // 0x8011D994
s32  fn_8011D9A0(u8* p);                  // 0x8011D9A0
s32  fn_8011D9A8(u8* p);                  // 0x8011D9A8
s32  fn_8011D9B4(u8* p);                  // 0x8011D9B4
void fn_8011D9C0(u8* p);                  // 0x8011D9C0
void fn_8011DF20(u8* p);                  // 0x8011DF20
void fn_8011E440(u8* p);                  // 0x8011E440
s32  fn_8011E94C(u8* p);                  // 0x8011E94C
s32  fn_8011E95C(u8* p);                  // 0x8011E95C
s32  fn_8011E970(u8* p);                  // 0x8011E970
s32  fn_8011E984(u8* p);                  // 0x8011E984
s32  fn_8011E998(u8* p);                  // 0x8011E998
}

extern "C" {

// StringHash constants (CRC-32 of the upper-cased name, see tools/hashes.py).
static const u32 kHashCreatures   = 0xE5368B8Bu;  // "creatures"
static const u32 kHashStatistics  = 0xE94C961Du;  // "Statistics"

typedef u8* (*HashMethod)(u8* self, u32 hash);

// Calls the virtual slot at byte offset `off` of obj with a hash argument; returns r3.
static u8* CallVirt(u8* obj, s32 off, u32 hash) {
    HashMethod fn = *(HashMethod*)(*(u8**)obj + off);
    return fn(obj, hash);
}

// lbl_805FB248 -> pointer -> +8 sub-object, then its vfunc 0x28(Statistics). NULL if missing.
static u8* StatisticsObj(void) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return 0;
    u8* sec = *(u8**)(top + 8);
    if (sec == 0) return 0;
    return CallVirt(sec, 0x28, kHashStatistics);
}

// 0x80113974
// Remote (opponent) data for the matchup screen: section +4 -> vfunc 0x28(creatures) -> +0x28.
void* MatchupGetRemoteData(void) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return 0;
    u8* sec = *(u8**)(top + 4);
    if (sec == 0) return 0;
    u8* found = CallVirt(sec, 0x28, kHashCreatures);
    if (found == 0) return 0;
    return *(void**)(found + 0x28);
}

// 0x80113A74
BOOL Statistics_GetPlayerRank(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011D79C(s);
}

// 0x80113AF0
BOOL Statistics_GetPlayerMSP(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011D71C(s);
}

// 0x80113B6C
BOOL Statistics_GetPlayerNextMSP(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011D728(s);
}

// 0x80113BE8
BOOL Statistics_GetPartMadness(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011D930(s);
}

// 0x80113C64
BOOL Statistics_GetMonsterMadness(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011D894(s);
}

// 0x80113CE0
BOOL Statistics_GetMonstersBuilt(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011D97C(s);
}

// 0x80113D5C
BOOL Statistics_GetBattlesWon(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011D988(s);
}

// 0x80113DD8
BOOL Statistics_GetBattlesLost(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011D994(s);
}

// 0x80113E54
BOOL Statistics_GetMonstersGoneMad(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011D9A0(s);
}

// 0x80113ED0
BOOL Statistics_GetExperimentSuccess(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011D9A8(s);
}

// 0x80113F4C
BOOL Statistics_GetExperimentFailure(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011D9B4(s);
}

// 0x80113FC8
void Statistics_SetupChallengeBestTimes(void) {
    u8* s = StatisticsObj();
    if (s == 0) return;
    fn_8011D9C0(s);
}

// 0x8011403C
void Statistics_SetupExperimentHighScores(void) {
    u8* s = StatisticsObj();
    if (s == 0) return;
    fn_8011DF20(s);
}

// 0x801140B0
void Statistics_SetupRegionCombatMSP(void) {
    u8* s = StatisticsObj();
    if (s == 0) return;
    fn_8011E440(s);
}

// 0x80114124
BOOL Statistics_GetDisplayScientistStats(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011E94C(s);
}

// 0x801141A0
BOOL Statistics_GetDisplayExperimentStats(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011E95C(s);
}

// 0x8011421C
BOOL Statistics_GetDisplayChallengeStats(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011E970(s);
}

// 0x80114298
BOOL Statistics_GetDisplayCombatStats(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011E984(s);
}

// 0x80114314
BOOL Statistics_GetExperimentPage1(void) {
    u8* s = StatisticsObj();
    if (s == 0) return 0;
    return fn_8011E998(s);
}

}
