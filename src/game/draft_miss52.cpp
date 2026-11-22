// Misc functions from the m2c draft batch "miss52": projectile/spell-effect helpers, fighter
// bookkeeping and HUD priority lists in 0x8001xxxx-0x8012xxxx plus two engine helpers.
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "game/part.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
s32   fn_8012F59C(void* world, s32 zone);                               // 0x8012F59C
void  fn_801B4C40(u8* ent);                                             // 0x801B4C40
BOOL  ZoneIsLoaded(void* world, s32 zone);                              // 0x8012F408
void* memcpy(void* d, const void* s, size_t n);
void* memset(void* p, int c, unsigned long n);
char* strchr(char* s, s32 c);                                           // SDK strchr
void  operator_delete(void* p);                                         // 0x801D426C
void* fn_801D40E0(s32 n);                                               // 0x801D40E0
s32   fn_80336BF8(u8* file, const char* fmt, ...);                      // 0x80336BF8
void  fn_8033B09C(void);                                                // 0x8033B09C
f64   fn_8033F1F4(f64 x);                                               // 0x8033F1F4 (arc-cosine-like, f64 in/out)
char* fn_80338BC0(char* s, const char* sub);                            // 0x80338BC0 (MSL strstr)
int   SysSprintf(char* buf, const char* fmt, ...);                      // 0x80142EFC
u32   StringHash(const char* s);                                        // 0x801EB704
u32   RandomU32(void);                                                  // 0x80143460
void  GameLog(int level, int category, const char* fmt, ...);           // 0x8000B2F0
void  EventSend(void*, void*, void*, u32, int, int, int, int);          // 0x801F3A84
u8*   GetGlobalSettings(void);                                          // 0x801D2BF8
void* HashTableFind(void* table, u32 hash);                             // 0x801DB32C
s32   WorldGetPhase(void* world);                                       // 0x800D83A8
void  WorldSendEvent(void* world, s32 id, int a, int b, int c);         // 0x800D87EC
void* ScreenSetsCurrent(void* sets);                                    // 0x8018D1E4
void* ScreenSetFindWidget(void* set, u32 hash);                         // 0x80198E34
void* ScreenSetFindElement(void* set, u32 hash);                        // 0x80198F18
BOOL  fn_80198C34(void* set, void* w);                                  // 0x80198C34
void  UISetText(void* el, const char* text, s32 a);                     // 0x8019EFC8
BOOL  fn_800E6BD4(void);                                                // 0x800E6BD4
void* fn_800E60BC(void);                                                // 0x800E60BC
void* fn_801F06FC(void);                                                // 0x801F06FC
void  GameFlowSignal(s32 id, s32 arg1, s32 arg2);                       // src/game/game_ui.cpp
s32 TriggerDoAction(void* action, void* a1, void* a2);                // 0x801E0054
void* SpawnHelper(void* owner, void* arg);                              // 0x8000C478 (arg is a pointer here, s32 elsewhere)
void  MonsterSetName(void* monster, const char* name);                  // 0x8021C8E0
s32   TextIdFromHash(u32 hash);                                         // 0x80120D34: -1 if unknown
const char* AnimSetName(s32 id);                                        // 0x8020E370
void  QuatFromMtx(Quat* out, const f32 m[12]);                          // 0x801456E4
s32   PartGetCurrentHealth(const PartInstance* p);                      // 0x80214A28
void  PartSetHealth(PartInstance* p, s32 hp);
void  LimbStatusInit(u8** field, PartInstance** part);                  // 0x80214A58

u8* fn_8020EAD4(u8* arg0);                                             // 0x8020EAD4
void  fn_8020EDF0(u8* dst, u8* src);                                    // 0x8020EDF0
void  fn_8020EB44(u8* p, s32 del);                                      // 0x8020EB44
void  fn_8003E000(u8* a, u8* b, u8* c);                                 // 0x8003E000
void  fn_80133698(void* world, s32 zone);                               // 0x80133698
s32   ZoneLoadAsync(void* world, s32 zone, int a, void (*done)(void*, void*), s32 arg);   // 0x80132098
void  fn_8003DB60(void* a, void* b);                                    // 0x8003DB60
s32   fn_8004FCB4(u8* obj, u8* other);                                    // 0x8004FCB4
s32   fn_8004EDA0(u8* a, u8* b);                                        // 0x8004EDA0
s32   fn_801B5338(u8* obj, u8* target, u32 hash, f32* vec, u8 flag);    // 0x801B5338
s32   fn_8004EED0(u8* arg0, u32 arg1, s32 arg2);                        // 0x8004EED0
void* fn_8004DAF8(void* c, s32 a, s32 b);                               // 0x8004DAF8
void* fn_8003DDE8(void* part, s32 slot, s32 def, s32 flag, s32 a);      // 0x8003DDE8 (returns the part's entity id)
void* fn_8004FFB4(void* id, void* part);                                // 0x8004FFB4
void  fn_8004C718(u8* p, s32 idx, void* elem);                          // 0x8004C718
void* fn_801B50FC(u8* p);                                               // 0x801B50FC (returns the object's transform matrix)
u8*   fn_801B6E74(u8* a3);                                              // 0x801B6E74
s32   fn_801ED538(u8* a3);                                              // 0x801ED538
void* fn_801B7198(void* anim, s32 a);                                   // 0x801B7198
void  fn_801ED624(u8* out, u8* src);                                    // 0x801ED624
u32   fn_801ED2FC(u8* a3);                                              // 0x801ED2FC
BOOL  fn_801C6634(u8* col);                                             // 0x801C6634
u8*   fn_801C668C(u8* key);                                             // 0x801C668C
void  fn_801ED554(u8* out, u8* color);                                  // 0x801ED554
void  fn_800699E4(u8* prop, u32 state, f32 t);                          // 0x800699E4
void  fn_80062DC4(u8* p, s32 a, s32 b);                                 // 0x80062DC4
void  fn_80062F58(u8* p);                                               // 0x80062F58 (small_8006.cpp)
void  fn_801C4B90(u8* obj, f32* a, f32* b, s32 c, s32 d, u8* e);        // 0x801C4B90
void  fn_801C3E7C(u8* tbl, u8* obj);                                    // 0x801C3E7C
s32   fn_801C3E88(u8* tbl, void* handle);                                 // 0x801C3E88
void  fn_801C5CC0(u8* obj, f32* a, f32* b);                             // 0x801C5CC0
void  fn_8020A398(s32 h);
u8*   fn_801AC184(u8* defs, s32 idx);                                   // 0x801AC184
void* fn_80007C54(void);                                                // 0x80007C54
void  fn_8018C0A8(u8* h, void* a);                                      // 0x8018C0A8
void* fn_801AC460(u8* anim, s32 a);                                     // 0x801AC460
void  fn_8018B2CC(u8* h, void* a, s32 b, s32 c, f32 d, f32 e);          // 0x8018B2CC
void  fn_8018A948(u8* h, f32 t);                                        // 0x8018A948
s32   fn_801AC51C(u8* p, u8* obj, u8* target);                              // 0x801AC51C
void  fn_8018C3D0(u8* h);                                               // 0x8018C3D0                                               // 0x8020A398
void  fn_801E839C(u8* p);                                               // 0x801E839C
void  fn_801DB398(u8* a, u32 hash, u8* b);                     // 0x801DB398
s32   fn_8012E7B0(u8* world, u8* obj, s32 a);                           // 0x8012E7B0
void  fn_800634D0(u8* h);                                               // 0x800634D0
void  fn_80063520(u8* h);                                               // 0x80063520
void  fn_801ED130(void* p);                                             // 0x801ED130
u32   fn_801EC880(u8* p, u32 hash);                                     // 0x801EC880
s32   fn_80138C40(void* world, u8* a, f32* b, u8* hit, u8* p, s32 c, s32 d);   // 0x80138C40
s32   fn_801C2B40(u8* a, u8* b, f32* c, f32 radius);                    // 0x801C2B40
s32   fn_80065200(u8* arg0, u8* arg1, void* arg2, u8* arg3, s32 arg4, s32 arg5, s32 arg6);   // 0x80065200
BOOL  fn_801F5220(void);                                                // 0x801F5220
void* fn_80060A28(u8* p);                                               // 0x80060A28
u8*   fn_801BC1C0(u8* node, s32 id);                                    // 0x801BC1C0
u8*   fn_801BC6F0(u8* base, s32 sub, s32 id);                           // 0x801BC6F0
BOOL  fn_801BC41C(u8* node, u8* mgr);                                   // 0x801BC41C
void  fn_801BBC9C(u8* mgr, u8* node, u8* found);                        // 0x801BBC9C

extern u8   gWorldState[];          // 0x803E6D00
extern u8   gGame[];                // 0x803B53A0
extern u8   __files[];              // 0x803B4978
extern u8   lbl_803C4298[];         // 0x803C4298 (+4: camera object with vtable)
extern u8   lbl_803C5240[];         // 0x803C5240
extern u8   lbl_8047C7AC[];         // 0x8047C7AC
extern s32  lbl_803C4E28[];         // 0x803C4E28 (combat part zone-loaded flags)
extern s32  lbl_805FB034;           // 0x805FB034
extern "C++" s32  lbl_805FB090;           // 0x805FB090 (call counter, small_8004b.cpp)
extern "C++" void* lbl_805FB094;          // 0x805FB094 (cached pointer, small_8004b.cpp)
extern "C++" u8   lbl_805FB098;           // 0x805FB098
extern u8   lbl_805FB099[];         // 0x805FB099
extern u8   lbl_805FB09C[];         // 0x805FB09C (per action ready flags)
extern u32  lbl_805FB0A0;           // 0x805FB0A0
extern s32  lbl_805FB0A4;           // 0x805FB0A4
extern u8   lbl_805FB0A8[];         // 0x805FB0A8
extern u8   lbl_805FB0A9[];         // 0x805FB0A9
extern u8   lbl_805FB0AA[];         // 0x805FB0AA
extern s32  lbl_805FB0B0[];         // 0x805FB0B0
extern s32  lbl_805FB0B8;           // 0x805FB0B8
extern u8   lbl_805FB0BC;           // 0x805FB0BC
extern u8   lbl_805FB0BD;           // 0x805FB0BD
extern u8   lbl_805FB0BE[];         // 0x805FB0BE
extern u8   lbl_805FB0BF;           // 0x805FB0BF
extern u8   lbl_805FB0C0;           // 0x805FB0C0
extern s32  lbl_805F8658;           // 0x805F8658
extern u8   lbl_805F865C;           // 0x805F865C
extern u8   lbl_805F865D[];         // 0x805F865D
extern u32  lbl_805F8708;           // 0x805F8708 (number of monster-name strings)
extern const char* const lbl_80389058[];   // 0x80389058 (reaction slot name table)
extern s32  lbl_805F84E0;           // 0x805F84E0
extern s32  lbl_805F84E4;           // 0x805F84E4
extern s32  lbl_805F84E8;           // 0x805F84E8
extern s32  lbl_805FB03C;           // 0x805FB03C
extern s32  lbl_805FB040;           // 0x805FB040
extern u8*  lbl_805FB014;           // 0x805FB014 (battle camera object)
extern u32  lbl_805FB9A4;           // 0x805FB9A4 (lock flag)
extern s32  lbl_805FB1D8[];         // 0x805FB1D8 (2 entries)
extern s32  lbl_805FB1E0[];         // 0x805FB1E0 (2 entries)
extern u8*  lbl_805FB2E8;           // 0x805FB2E8
extern f32  lbl_805FD25C;           // 0x805FD25C
extern f32  lbl_805FD268;           // 0x805FD268
extern "C++" struct StringTable* gStringTable;           // 0x805FB910
}

extern "C++" void  MonsterGenerateOpponent(void* builder, s32 tmplIndex);   // 0x8021A1D0 (opponent_gen.cpp)
extern "C++" void  UpdateAllZoneEntities(void);                             // game_flow.cpp
extern "C++" s32   gZoneResourceSize;   // 0x805FB94C
extern "C++" void* gScreenSets;         // 0x805FB654
extern "C++" void* gEventMgr;           // 0x805FB6C8
extern "C++" s32   gSelectedPart;       // 0x805F8650
extern "C++" s32   gHighlightedAction;  // 0x805F8654
extern "C++" void* gStringTable2;       // 0x805FB90C

static const u32 kHashExplorationScreen = 0xDB1E4D0Cu;  // "exploration_screen"
static const u32 kHashTimerGroup = 0x9ACB282Du;         // "timer_group"

extern "C" {
// 0x800126A0
// Gathers the zone-1 entity list into `out`: first the +0x90 block (count at +0x98), then the
// +0x8C block (count at +0x94); stores counts in p+0x4/+0x58 and total in p+0x8 (0 if it
// would not fit in `cap`). Pokes every loaded entity that has a non-zero field +4 first.
void fn_800126A0(u8* p, s32 unused, u8* out, u32 cap) {
    u8* z = *(u8**)(*(u8**)gWorldState + 0x1a4) + (s32)(1 * gZoneResourceSize);
    u32 total = 0;
    if (fn_8012F59C(gWorldState, 1) != 0 || ZoneIsLoaded(gWorldState, 1) != 0) {
        u8* zz = *(u8**)(*(u8**)gWorldState + 0x1a4) + (s32)(1 * gZoneResourceSize);
        s32 n = *(s32*)(zz + 0x94) + *(s32*)(zz + 0x98);
        for (s32 i = 0; i < n; i++) {
            u8* zr = *(u8**)(*(u8**)gWorldState + 0x1a4) + (s32)(1 * gZoneResourceSize);
            s32 split = *(s32*)(zr + 0x94);
            u8* ent;
            if (i < split) ent = *(u8**)(*(u8**)(zr + 0x120) + i * 4);
            else           ent = *(u8**)(*(u8**)(zr + 0x12c) + (i - split) * 4);
            if (ent != NULL && *(s32*)(ent + 4) != 0) fn_801B4C40(ent);
        }
    }
    *(s32*)(p + 0x4) = 0;
    u32 sum = (u32)(*(s32*)(z + 0x98) + *(s32*)(z + 0x94));
    *(s32*)(p + 0x58) = *(s32*)(z + 0x98);
    if (sum < cap) {
        memcpy(out, *(void**)(z + 0x90), (u32)*(s32*)(z + 0x98) << 6);
        memcpy(out + ((u32)(*(s32*)(p + 0x4) + *(s32*)(p + 0x58)) << 6), *(void**)(z + 0x8c),
               (u32)*(s32*)(z + 0x94) << 6);
        total = sum;
    }
    *(u32*)(p + 0x8) = total;
}

// 0x80012CB0
// Per-frame countdown timer shown in the "timer_group" UI group (time left at +0x200, shown
// flag at +0x208). Counts down in game state 5 / world phase 2; phase 1 resets and hides it.
void fn_80012CB0(u8* p, f32 dt) {
    if (!(*(f32*)(p + 0x200) > 0.0f)) return;
    s32 v = *(s32*)(gGame + 0xD620);
    if (v == 0) v = *(s32*)(gGame + 0xD5F0);
    if (v == 0) return;
    s32 phase = WorldGetPhase(gGame + 0xD5F0);
    void* set = ScreenSetsCurrent(gScreenSets);
    u8* el = NULL;
    s32 show = 0;
    if (set != NULL && fn_80198C34(set, ScreenSetFindWidget(set, kHashExplorationScreen)) &&
        *(u8*)(p + 0x208) != 0) {
        UIElement_SetVisibleFlag(kHashTimerGroup, 1);
        el = (u8*)ScreenSetFindElement(set, 0x52B01273u);
    }
    if (fn_800E6BD4() == 0 && *(s32*)(lbl_803C4298 + 0x18) == 5 && phase == 2) {
        f32 t = *(f32*)(p + 0x200) - dt;
        *(f32*)(p + 0x200) = t;
        if (t < 0.0f) *(f32*)(p + 0x200) = 0.0f;
        if (*(f32*)(p + 0x200) > 0.0f && *(u8*)(p + 0x208) != 0) show = 1;
    } else if (phase == 1) {
        *(u8*)(p + 0x208) = 0;
        *(f32*)(p + 0x200) = -1.0f;
        void* set2 = ScreenSetsCurrent(gScreenSets);
        if (set2 != NULL && fn_80198C34(set2, ScreenSetFindWidget(set2, kHashExplorationScreen)))
            UIElement_SetVisibleFlag(kHashTimerGroup, 0);
        return;
    }
    if (show != 0 && el != NULL) {
        if (!(*(u32*)(el + 0x64) & 2))
            (*(void (**)(u8*, s32))(*(u8**)(el + 0x138) + 0x2c))(el, 2);
        f32 t = *(f32*)(p + 0x200);
        s32 mins = (s32)(t / 60.0f);
        char buf[32];
        SysSprintf(buf, "%d:%02d", mins, (s32)(t - (f32)(mins * 60)));
        UISetText(el, buf, 1);
        return;
    }
    if (el != NULL && (*(u32*)(el + 0x64) & 2)) {
        (*(void (**)(u8*, s32))(*(u8**)(el + 0x138) + 0x30))(el, 2);
        UIElement_SetVisibleFlag(kHashTimerGroup, 0);
    }
}

// 0x800311E8
// vector<Vec2>::push_back for the container {data, size, capacity} (8-byte elements). Grows by
// about 1.6x/1.5x when full, with the MSL "vector length error" / allocation checks.
void fn_800311E8(u8* vec, f32* pt) {
    f32 x = pt[0], y = pt[1];
    u32 size = *(u32*)(vec + 4);
    u32 cap = *(u32*)(vec + 8);
    if (size < cap) {
        u8* data = *(u8**)(vec + 0);
        *(u32*)(vec + 4) = size + 1;
        *(f32*)(data + size * 8) = x;
        *(f32*)(data + size * 8 + 4) = y;
        return;
    }
    if (0x1FFFFFFFu - cap < 1u) {
        fn_80336BF8(__files + 0xA0, "vector length error");
        fn_8033B09C();
    }
    u32 need = (*(u32*)(vec + 4) + 1) - *(u32*)(vec + 8);
    u32 oldcap = *(u32*)(vec + 8);
    if (need > 0x1FFFFFFFu - oldcap) {
        fn_80336BF8(__files + 0xA0, "vector length error");
        fn_8033B09C();
    }
    u32 newcap;
    if (oldcap < 0x0AAAAAAAu) {
        u32 g = (((oldcap + 1) * 4) - (oldcap + 1)) / 5;
        newcap = oldcap + (g < need ? need : g);
    } else if (oldcap < 0x15555554u) {
        u32 g = (oldcap + 1) >> 1;
        newcap = oldcap + (g < need ? need : g);
    } else {
        newcap = 0x1FFFFFFFu;
    }
    if (newcap > 0x1FFFFFFFu) {
        fn_80336BF8(__files + 0xA0, "vector length error");
        fn_8033B09C();
    }
    u8* nd = (u8*)fn_801D40E0((s32)(newcap * 8));
    if (nd == NULL) {
        fn_80336BF8(__files + 0xA0, "Memory allocation failure");
        fn_8033B09C();
    }
    u32 sz = *(u32*)(vec + 4);
    u8* old = *(u8**)(vec + 0);
    *(f32*)(nd + sz * 8) = x;
    *(f32*)(nd + sz * 8 + 4) = y;
    for (u32 i = 0; i < sz; i++) {
        *(f32*)(nd + i * 8) = *(f32*)(old + i * 8);
        *(f32*)(nd + i * 8 + 4) = *(f32*)(old + i * 8 + 4);
    }
    *(u32*)(vec + 4) = 0;
    *(u8**)(vec + 0) = nd;
    *(u32*)(vec + 8) = newcap;
    *(u32*)(vec + 4) = sz + 1;
    if (old != NULL) operator_delete(old);
}

// 0x8003D964
// Builds two 0x13C-byte records from the game-db template at (db+0xC)->+0x9180 + 0xD8 (0x48 bytes
// each; the first with five fields overridden by globals), tags both with 6 at +0x44, hands
// them to fn_8003E000 and destroys them.
void fn_8003D964(u8* obj) {
    u8 a[0x13C];
    u8 b[0x13C];
    u8 t1[0x48];
    u8 t2[0x48];
    fn_8020EAD4(a);
    fn_8020EAD4(b);
    u8* tmpl = *(u8**)(gGameDb->data + 0x9180);
    memcpy(t1, tmpl + 0xd8, 0x48);
    *(s32*)(t1 + 0x14) = lbl_805F84E0;
    *(s32*)(t1 + 0x1c) = lbl_805FB03C;
    *(s32*)(t1 + 0x30) = lbl_805FB040;
    *(s32*)(t1 + 0x2c) = lbl_805F84E8;
    *(s32*)(t1 + 0x0c) = lbl_805F84E4;
    memcpy(t2, tmpl + 0xd8, 0x48);
    fn_8020EDF0(a, t1);
    *(s32*)(a + 0x44) = 6;
    fn_8020EDF0(b, t2);
    *(s32*)(b + 0x44) = 6;
    fn_8003E000(obj, a, b);
    fn_8020EB44(b, -1);
    fn_8020EB44(a, -1);
}

// 0x8003DB78
// Looks up the resource-table entry whose +4 equals `hash` (searching from index 0, or index `a`
// when b == 0), claims it (count++ at +0), and for b == 0 kicks off an async zone load.
// Returns the entry's object, or the hashed fallback when only a busy entry was found.
void* fn_8003DB78(u32 hash, s32 a, s32 b) {
    u8* obj = NULL;
    s32 busy = 0;
    s32 idx = 0;
    u8* q = **(u8***)((u8*)fn_801F06FC() + 8);
    s32 n = *(s32*)(q + 0x48);
    for (s32 i = 0; i < n; i++) {
        u8* ent = *(u8**)(*(u8**)(q + 0x44) + i * 4);
        if (ent != NULL && hash == *(u32*)(ent + 4)) {
            if (b == 0) idx += a;
            u8* slot = *(u8**)(*(u8**)(q + 0x44) + idx * 4);
            s32 cnt = *(s32*)slot;
            if (cnt <= 0) {
                *(s32*)slot = cnt + 1;
                u8* e2 = *(u8**)(*(u8**)(q + 0x44) + idx * 4);
                obj = *(u8**)(e2 + 8);
                if (b == 0) {
                    s32 zone = *(s32*)(e2 + 0xc);
                    fn_80133698(gWorldState, zone);
                    lbl_803C4E28[lbl_805FB034] = 0;
                    ZoneLoadAsync(gWorldState, zone, 1, fn_8003DB60, lbl_805FB034);
                    lbl_805FB034 += 1;
                }
            } else {
                busy = *(s32*)(slot + 8);
            }
            break;
        }
        idx++;
    }
    if (obj == NULL && busy != 0) {
        u8* zr = *(u8**)(*(u8**)gWorldState + 0x1a4) + gZoneResourceSize;
        obj = (u8*)HashTableFind(zr + 0xe4, 0x2c733c94u);
        fn_8004FCB4(obj, (u8*)(intptr_t)busy);
    }
    return obj;
}

// 0x80044CDC
// Resets the combat part-selection HUD state (lbl_803C5240 block and the 0x805FB0xx flags) and,
// if the battle camera object has a view record (+0x4D4), copies its two pose triples/quads.
void* fn_80044CDC(void* arg) {
    u8* b = lbl_803C5240;
    lbl_805FB090 = 0;
    lbl_805FB094 = NULL;
    gSelectedPart = -1;
    lbl_805FB098 = 0;
    *(u8*)(b + 0x60) = 0;
    *(s32*)(b + 0x64) = 4;
    *(s32*)(b + 0x5c) = 0x4e;
    lbl_805FB099[0] = 0;
    *(f32*)(b + 0x68) = 0.0f;
    *(f32*)(b + 0x6c) = 0.0f;
    *(f32*)(b + 0x70) = 0.0f;
    *(f32*)(b + 0x74) = 0.0f;
    *(f32*)(b + 0x78) = 0.0f;
    *(f32*)(b + 0x7c) = 0.0f;
    *(f32*)(b + 0x80) = 0.0f;
    *(f32*)(b + 0x84) = 0.0f;
    *(f32*)(b + 0x88) = 0.0f;
    *(f32*)(b + 0x8c) = 1.0f;
    *(f32*)(b + 0x90) = 0.0f;
    *(f32*)(b + 0x94) = 0.0f;
    *(f32*)(b + 0x98) = 0.0f;
    *(f32*)(b + 0x9c) = 1.0f;
    gHighlightedAction = -1;
    lbl_805FB0A0 = 0;
    lbl_805FB0A4 = 0;
    lbl_805FB0A8[0] = 0;
    lbl_805FB0A9[0] = 0;
    lbl_805FB0AA[0] = 0;
    lbl_805F8658 = -1;
    lbl_805FB0B8 = 0;
    lbl_805FB0BC = 0;
    lbl_805FB0BD = 0;
    lbl_805F865C = 1;
    lbl_805FB0BE[0] = 0;
    lbl_805F865D[0] = 1;
    lbl_805FB0BF = 0;
    lbl_805FB0C0 = 0;
    lbl_805FB09C[0] = 1;
    lbl_805FB09C[1] = 1;
    lbl_805FB0B0[0] = 1;
    for (s32 i = 0; i < 10; i++) *(s32*)(b + 0xa0 + i * 4) = -1;
    lbl_805FB0B0[1] = 1;
    *(s32*)(b + 0xc8) = 0;
    *(s32*)(b + 0xcc) = 0;
    *(s32*)(b + 0xd0) = 0;
    *(s32*)(b + 0xd4) = 0;
    for (s32 i = 0; i < 10; i++) *(s32*)(b + 0x28 + i * 4) = -1;
    u8* cam = *(u8**)(lbl_805FB014 + 0x4d4);
    if (cam != NULL) {
        u8* v = *(u8**)(cam + 4);
        *(f32*)(b + 0x68) = *(f32*)(v + 0x0);
        *(f32*)(b + 0x6c) = *(f32*)(v + 0x4);
        *(f32*)(b + 0x70) = *(f32*)(v + 0x8);
        v = *(u8**)(cam + 4);
        *(f32*)(b + 0x74) = *(f32*)(v + 0x0);
        *(f32*)(b + 0x78) = *(f32*)(v + 0x4);
        *(f32*)(b + 0x7c) = *(f32*)(v + 0x8);
        v = *(u8**)(cam + 4);
        *(f32*)(b + 0x80) = *(f32*)(v + 0xc);
        *(f32*)(b + 0x84) = *(f32*)(v + 0x10);
        *(f32*)(b + 0x88) = *(f32*)(v + 0x14);
        *(f32*)(b + 0x8c) = *(f32*)(v + 0x18);
        v = *(u8**)(cam + 4);
        *(f32*)(b + 0x90) = *(f32*)(v + 0xc);
        *(f32*)(b + 0x94) = *(f32*)(v + 0x10);
        *(f32*)(b + 0x98) = *(f32*)(v + 0x14);
        *(f32*)(b + 0x9c) = *(f32*)(v + 0x18);
    }
    return arg;
}

// 0x8004B938
// For each of the 5 fighters (+0xC50 table) and each other fighter j, registers a zero-vector
// relation between their parts via fn_801B5338 unless fn_8004EDA0 reports one already exists.
void fn_8004B938(u8* p) {
    f32 zero = 0.0f;
    for (s32 i = 0; i < 5; i++) {
        u8* fi = *(u8**)(p + 0xc50 + i * 4);
        if (fi == NULL) continue;
        u8* q = (u8*)(*(u8*(**)(u8*))(*(u8**)fi + 0xcc))(fi);
        u8* partA = *(u8**)(q + 0x20);
        if (partA != NULL) partA -= 0x264;
        for (s32 j = 0; j < 5; j++) {
            if (j == i) continue;
            u8* fj = *(u8**)(q + 0xc + j * 4);
            if (fj == NULL) continue;
            u8* partB = *(u8**)(*(u8**)(fj + 4) + 0x20);
            if (partB != NULL) partB -= 0x264;
            if (fn_8004EDA0(partA, partB) == 0) {
                f32 v[3];
                v[2] = zero;
                v[1] = zero;
                v[0] = zero;
                const char* name = (*(const char*(**)(u8*, s32))(*(u8**)fj + 0xc))(fj, j);
                fn_801B5338(partA, partB, StringHash(name), v, 0);
            }
        }
    }
}

// sum of the 13 global-settings counters at +0x34..+0x64 (all s32)
static s32 SettingsFighterTotal(void) {
    s32 a = *(s32*)(GetGlobalSettings() + 0x4c);
    s32 b = *(s32*)(GetGlobalSettings() + 0x34);
    s32 c = *(s32*)(GetGlobalSettings() + 0x50);
    b += a;
    a = *(s32*)(GetGlobalSettings() + 0x54);
    b = c + b;
    c = *(s32*)(GetGlobalSettings() + 0x58);
    b = a + b;
    a = *(s32*)(GetGlobalSettings() + 0x5c);
    b = c + b;
    c = *(s32*)(GetGlobalSettings() + 0x60);
    b = a + b;
    a = *(s32*)(GetGlobalSettings() + 0x64);
    b = c + b;
    c = *(s32*)(GetGlobalSettings() + 0x38);
    b = a + b;
    a = *(s32*)(GetGlobalSettings() + 0x3c);
    b = c + b;
    c = *(s32*)(GetGlobalSettings() + 0x40);
    b = a + b;
    a = *(s32*)(GetGlobalSettings() + 0x44);
    b = c + b;
    s32 d = *(s32*)(GetGlobalSettings() + 0x48);
    return d + (a + b);
}

// 0x8004CEE8
// For every slot 0..total (total = sum of the settings counters), finds the zone assigned to it
// (slot == total -> zone 0; else the +0xB0 table entry when its kind is 6) and calls virtual
// +0x18 on each of that zone's entities flagged 0x2000 with a non-zero low-2-bit mask.
void fn_8004CEE8(void* arg) {
    for (s32 i = 0; i < SettingsFighterTotal() + 1; i++) {
        s32 zone = -1;
        if (i == SettingsFighterTotal()) {
            zone = 0;
        } else {
            u8* e = *(u8**)(gWorldState + 0xb0) + i * 0x30;
            if (*(s32*)(e + 4) == 6) zone = *(s32*)(e + 8);
        }
        if (zone < 0) continue;
        u8* zr = *(u8**)(*(u8**)gWorldState + 0x1a4) + zone * gZoneResourceSize;
        s32 n = *(s32*)(zr + 0x94) + *(s32*)(zr + 0x98);
        for (s32 k = 0; k < n; k++) {
            zr = *(u8**)(*(u8**)gWorldState + 0x1a4) + zone * gZoneResourceSize;
            s32 split = *(s32*)(zr + 0x94);
            u8* ent;
            u8* rec;
            if (k < split) {
                ent = *(u8**)(*(u8**)(zr + 0x120) + k * 4);
                rec = *(u8**)(zr + 0x8c) + k * 0x40;
            } else {
                s32 m = k - split;
                ent = *(u8**)(*(u8**)(zr + 0x12c) + m * 4);
                rec = *(u8**)(zr + 0x90) + m * 0x40;
            }
            if ((*(u32*)(rec + 0x1c) & 0x2000) && (*(u32*)(rec + 0x20) & 3) && ent != NULL)
                (*(void (**)(u8*, void*))(*(u8**)ent + 0x18))(ent, arg);
        }
    }
}

// 0x8004D7E0
// Spawns a random opponent: sets the speed constant (6.0 or 3.5 depending on spec flag 0x4000),
// generates the monster, then names it from STRING_MONSTER_NAME%03d (random 1..N, N = lbl_805F8708).
void* fn_8004D7E0(u8* mon, s32 tmplIndex, s32 a2) {
    if (!(*(u32*)(*(u8**)(mon + 4) + 0x1c) & 0x4000)) *(f32*)(mon + 0x9f4) = lbl_805FD25C;
    else *(f32*)(mon + 0x9f4) = lbl_805FD268;
    MonsterGenerateOpponent(mon + 0x898, tmplIndex);
    *(u8*)(mon + 0xc84) = 1;
    char buf[0x100];
    memset(buf, 0, 0x100);
    u32 r = RandomU32();
    SysSprintf(buf, "STRING_MONSTER_NAME%03d", (s32)(r % (u32)lbl_805F8708) + 1);
    s32 id = TextIdFromHash(StringHash(buf));
    const char* name;
    if (id == -1) {
        name = "Not found";
    } else if ((id & 0x80000000) && gStringTable2 != NULL) {
        u8* t = (u8*)gStringTable2;
        const char** strs = *(const char***)t;
        if (strs != NULL && ((u32)id & 0x7FFFFFFFu) < *(u32*)(t + 0xc)) name = strs[(u32)id & 0x7FFFFFFFu];
        else name = "";
    } else {
        const char** strs = *(const char***)gStringTable;
        if (strs != NULL && (u32)id < *(u32*)((u8*)gStringTable + 0xc)) {
            name = strs[(u32)id];
        } else {
            name = "";
            if (id == -1) name = "NOT FOUND";
        }
    }
    if (name != NULL) MonsterSetName(mon + 0x898, name);
    if (*(u32*)(*(u8**)(mon + 4) + 0x20) & 0x200000) return fn_8004DAF8(mon, 0, a2);
    return NULL;
}

// 0x8004E060
// Applies `delta` damage (negative = heal) to fighter `slot`'s part, creating its part instance
// first if needed (phase 2 + heal), then signals when every limb part is dead.
void fn_8004E060(u8* p, s32 slot, s32 delta) {
    u8* tbl = *(u8**)(p + 0xc4c);
    if (tbl == NULL) return;
    s32 off = slot * 4;
    u8** cur = *(u8***)(tbl + off);
    if (WorldGetPhase(gGame + 0xD5F0) == 2 && delta < 0 && cur == NULL) {
        u8* pa = p + off;
        s32* part = *(s32**)(pa + 0x89c);
        s32 kind = *part;
        if (kind != -1) {
            u8* r = (u8*)fn_8003DDE8(part, slot, kind, 1, 0);
            *(u8**)(pa + 0xc6c) = r;
            if (r != NULL) {
                if ((intptr_t)r < 0x3e8) {
                    *(u8**)(pa + 0xc6c) =
                        *(u8**)(*(u8**)(*(u8**)gWorldState + 0x1a4) + ((s32)(intptr_t)r - 1) * gZoneResourceSize + 0x10c);
                }
                u8* q = (u8*)fn_8004FFB4(*(u8**)(pa + 0xc6c), *(u8**)(pa + 0x89c));
                if (q != NULL) {
                    fn_8004C718(p, slot, q);
                    UpdateAllZoneEntities();
                }
            }
        }
        cur = *(u8***)(*(u8**)(p + 0xc4c) + off);
    }
    if (cur != NULL) {
        s32 hp = PartGetCurrentHealth(*(PartInstance**)(p + off + 0x89c)) - delta;
        if (hp < 0) hp = 0;
        u8* pa = p + off;
        PartSetHealth(*(PartInstance**)(pa + 0x89c), hp);
        PartInstance* part = *(PartInstance**)(pa + 0x89c);
        *(PartInstance**)cur = part;
        if (part != NULL) LimbStatusInit((u8**)((u8*)cur + 0x28), (PartInstance**)cur);
    }
    if (p == *(u8**)(gGame + 0xD614)) {
        if (delta > 0) WorldSendEvent(gGame + 0xD5F0, 0x520CC75C, 0, 0, 0);
        PartInstance* t = *(PartInstance**)(p + 0x8ac);
        s32 allDead = 0;
        if (t == NULL || PartGetCurrentHealth(t) <= 0) {
            allDead = 1;
        } else {
            PartInstance* a = *(PartInstance**)(p + 0x89c);
            PartInstance* b = *(PartInstance**)(p + 0x8a4);
            PartInstance* c = *(PartInstance**)(p + 0x8a8);
            PartInstance* d = *(PartInstance**)(p + 0x8a0);
            if ((a == NULL || PartGetCurrentHealth(a) <= 0) && (b == NULL || PartGetCurrentHealth(b) <= 0) &&
                (c == NULL || PartGetCurrentHealth(c) <= 0) && (d == NULL || PartGetCurrentHealth(d) <= 0))
                allDead = 1;
        }
        if (allDead != 0 && WorldGetPhase(gGame + 0xD5F0) == 2) GameFlowSignal(0x06B0A31C, 0, 0);
    }
}

// 0x8004F0DC
// Builds the animation name for a reaction slot: finds which prefix from the slot-name table
// occurs in `name`; if it is the object's own slot (+4 of vcall 0xCC), uses `name` as is,
// otherwise composes "<slot>_<animset>_s_<name>" and passes the hash to fn_8004EED0.
s32 fn_8004F0DC(void* obj, const char* name, s32 arg2) {
    u8* o = (u8*)obj;
    s32 found = -1;
    for (s32 i = 0; i < 5; i++) {
        if (fn_80338BC0((char*)name, (char*)lbl_80389058[i]) != NULL) found = i;
    }
    typedef u8* (*GetFn)(u8*);
    GetFn get = *(GetFn*)(*(u8**)o + 0xcc);
    s32 slotIdx = *(s32*)(get(o) + 4);
    char buf[0x90];
    if (found != slotIdx) {
        u8* a = get(o);
        if (a == NULL || *(s32*)a == 0) return 0;
        s32 kind = **(s32**)get(o);
        s32 setIdx;
        if (kind != -1) {
            PartDef def;
            PartDbGetDef(&def, gGameDb, kind);
            setIdx = def.unk34;
        } else if (strchr((char*)o + 0x108, '_') == NULL ||
                   strchr(strchr((char*)o + 0x108, '_') + 1, '_') == NULL) {
            setIdx = -1;
        } else {
            char* us = strchr(strchr((char*)o + 0x108, '_') + 1, '_');
            setIdx = 0;
            for (;;) {
                const char* nm = AnimSetName(setIdx);
                if ((s8)us[1] == (s8)nm[0] && (s8)us[2] == (s8)nm[1] && (s8)us[3] == (s8)nm[2]) break;
                setIdx++;
                if (setIdx >= 0x15) {
                    setIdx = -1;
                    break;
                }
            }
        }
        const char* animName = AnimSetName(setIdx);
        SysSprintf(buf, "%s_%s_s_%s", lbl_80389058[slotIdx], animName, name);
    } else {
        SysSprintf(buf, "%s", name);
    }
    return fn_8004EED0(o, StringHash(buf), arg2);
}

// Inserts a 0x20-byte entry into the 4-slot priority list at p+0xB04, ordered by the f32 at
// entry+0x1C (higher first); a lower-priority tail entry is pushed down, a duplicate id is ignored.
static void HudListInsert(u8* p, const u8* e) {
    u32 id = *(u32*)e;
    f32 prio = *(f32*)(e + 0x1c);
    for (s32 i = 0; i < 4; i++) {
        u8* cur = p + 0xB04 + i * 0x20;
        if (*(f32*)(cur + 0x1c) < prio) {
            for (s32 k = 2; k >= i; k--) memcpy(p + 0xB04 + (k + 1) * 0x20, p + 0xB04 + k * 0x20, 0x20);
            memcpy(p + 0xB04 + i * 0x20, e, 0x20);
            return;
        }
        if (id == *(u32*)cur) return;
    }
}

// 0x8005F81C
// Queues an entry with id `id` and priority `prio` (all other fields zero, byte at +8 = 0xFF).
void fn_8005F81C(u8* p, u32 id, f32 prio) {
    u8 e[0x20];
    memset(e, 0, 0x20);
    *(u32*)(e + 0x0) = id;
    *(u8*)(e + 0x8) = 0xFF;
    *(f32*)(e + 0x1c) = prio;
    HudListInsert(p, e);
}

// 0x8005FAAC
// Same as fn_8005F81C but also stores `extra` at entry+0x10.
void fn_8005FAAC(u8* p, s32 extra, u32 id, f32 prio) {
    u8 e[0x20];
    memset(e, 0, 0x20);
    *(u32*)(e + 0x0) = id;
    *(u8*)(e + 0x8) = 0xFF;
    *(f32*)(e + 0x1c) = prio;
    *(s32*)(e + 0x10) = extra;
    HudListInsert(p, e);
}

// 0x80060004
// Removes the entry with id `id` from the 8-slot list at p+0xA04 (later entries shift up and
// the last slot's priority is cleared).
void fn_80060004(u8* p, u32 id) {
    for (s32 i = 0; i < 8; i++) {
        if (id == *(u32*)(p + 0xA04 + i * 0x20)) {
            for (s32 k = i; k < 7; k++) memcpy(p + 0xA04 + k * 0x20, p + 0xA04 + (k + 1) * 0x20, 0x20);
            *(f32*)(p + 0xB00) = 0.0f;
            return;
        }
    }
}

// 0x800603AC
// Queues the fixed-id (hash 0x7A6B66E7) entry; the 4 bytes at +0xC come from fn_801ED554(arg).
void fn_800603AC(u8* p, u8* arg, f32 prio) {
    u8 e[0x20];
    u8 tmp[4];
    memset(e, 0, 0x20);
    *(u32*)(e + 0x0) = 0x7A6B66E7u;
    *(u8*)(e + 0x8) = 0xFF;
    *(f32*)(e + 0x1c) = prio;
    fn_801ED554(tmp, arg);
    *(u16*)(e + 0xe) = *(u16*)(tmp + 2);
    *(u8*)(e + 0xd) = tmp[1];
    *(u8*)(e + 0xc) = tmp[0];
    HudListInsert(p, e);
}

// 0x8006066C
// Queues the fixed-id (0x4553808C) entry carrying an int at +0x14, a float at +0x18 and priority.
void fn_8006066C(u8* p, s32 value, f32 extra, f32 prio) {
    u8 e[0x20];
    memset(e, 0, 0x20);
    *(u32*)(e + 0x0) = 0x4553808Cu;
    *(u8*)(e + 0x8) = 0xFF;
    *(f32*)(e + 0x1c) = prio;
    *(f32*)(e + 0x18) = extra;
    *(s32*)(e + 0x14) = value;
    HudListInsert(p, e);
}

// Calls the "is disabled/ignored" virtual (+0x4C) of an entity.
static s32 EntityVirt4C(u8* o) { return (*(s32 (**)(u8*))(*(u8**)o + 0x4c))(o); }

// Looks up the 8-slot {id, flag} table at p+0xBE0 for the entity's id (+0x9C -> +8); 0 if absent.
static u8 EntityTableFlag(u8* p, u8* o) {
    u32 id = 0;
    u8* sub = *(u8**)(o + 0x9c);
    if (sub != NULL) id = *(u32*)(sub + 8);
    for (s32 i = 0; i < 8; i++) {
        if (id == *(u32*)(p + 0xBE0 + i * 8)) return *(u8*)(p + 0xBE4 + i * 8);
    }
    return 0;
}

// 0x80060A28
// Finds the entity closest to `p` among the 32 gGame entities and the 2 world slots (+0x10 table)
// that are enabled and flagged in p's table; returns it (or NULL).
void* fn_80060A28(u8* p) {
    f32 best = 1000000.0f;
    u8* bestObj = NULL;
    for (s32 i = 0; i < 0x20; i++) {
        u8* o = *(u8**)(gGame + 0x140 + i * 4);
        if (o != NULL && EntityVirt4C(o) == 0) {
            u8* e = *(u8**)(gGame + 0x140 + i * 4);
            if (e != p) {
                if (EntityTableFlag(p, e) != 0) {
                    u8* a = *(u8**)(e + 4);
                    u8* b = *(u8**)(p + 4);
                    f32 dx = *(f32*)(a + 0) - *(f32*)(b + 0);
                    f32 dy = *(f32*)(a + 4) - *(f32*)(b + 4);
                    f32 dz = *(f32*)(a + 8) - *(f32*)(b + 8);
                    f32 d = dz * dz + (dx * dx + dy * dy);
                    if (d < best) {
                        bestObj = e;
                        best = d;
                    }
                }
            }
        }
    }
    for (s32 i = 0; i < 2; i++) {
        u8* e = *(u8**)(*(u8**)gWorldState + i * 4 + 0x10);
        if (e != NULL && (*(u32*)(*(u8**)(e + 4) + 0x20) & 1) && EntityVirt4C(e) == 0) {
            if (EntityTableFlag(p, e) != 0) {
                u8* a = *(u8**)(e + 4);
                u8* b = *(u8**)(p + 4);
                f32 dx = *(f32*)(a + 0) - *(f32*)(b + 0);
                f32 dy = *(f32*)(a + 4) - *(f32*)(b + 4);
                f32 dz = *(f32*)(a + 8) - *(f32*)(b + 8);
                f32 d = dz * dz + (dx * dx + dy * dy);
                if (d < best) {
                    bestObj = e;
                    best = d;
                }
            }
        }
    }
    return bestObj;
}

// 0x80061024
// True when the angle between the vector to the target (the vec at +0xB88 if flag 8 at +0xA08 is
// set, else the object at +0xA14; flattened to XY) and the facing vector (fn_801B50FC) is below
// `deg` (the original compares acos(cos) * 2/pi against deg in radians).
s32 fn_80061024(u8* p, f32 deg) {
    f32 vx, vy, vz;
    if (*(u32*)(p + 0xa08) & 8) {
        u8* pos = *(u8**)(p + 4);
        vx = *(f32*)(p + 0xb88) - *(f32*)(pos + 0);
        vy = *(f32*)(p + 0xb8c) - *(f32*)(pos + 4);
        vz = *(f32*)(p + 0xb90) - *(f32*)(pos + 8);
    } else {
        u8* t = *(u8**)(p + 0xa14);
        if (t == NULL) return 0;
        u8* tp = *(u8**)(t + 4);
        u8* pos = *(u8**)(p + 4);
        vx = *(f32*)(tp + 0) - *(f32*)(pos + 0);
        vy = *(f32*)(tp + 4) - *(f32*)(pos + 4);
        vz = *(f32*)(tp + 8) - *(f32*)(pos + 8);
    }
    vz = 0.0f;
    u8* f = (u8*)fn_801B50FC(p);
    f32 fx = *(f32*)(f + 0xc);
    f32 fy = *(f32*)(f + 0x10);
    f32 fz = 0.0f;
    f32 l1sq = (vz * vz) + ((vx * vx) + (vy * vy));
    f32 l1 = 0.0f;
    if (!(l1sq <= 0.0f)) l1 = sqrtf(l1sq);  // original: frsqrte with two Newton steps
    f32 l2sq = (fz * fz) + ((fx * fx) + (fy * fy));
    f32 l2 = 0.0f;
    if (!(l2sq <= 0.0f)) l2 = sqrtf(l2sq);
    f32 prod = l1 * l2;
    f32 ang = 0.0f;
    if (!(prod <= 0.0f)) {
        f32 c = ((vz * 0.0f) + ((vx * fx) + (vy * fy))) / prod;
        f32 cl = c;
        if (!(c < -1.0f)) cl = (c > 1.0f) ? 1.0f : c; else cl = -1.0f;
        f32 c2 = cl;
        f32 cl2;
        if (!(c2 < -1.0f)) cl2 = (c2 > 1.0f) ? 1.0f : c2; else cl2 = -1.0f;
        ang = 0.63661975f * (f32)fn_8033F1F4((f64)cl2);
    }
    if (ang < 0.017453292f * deg) return 1;
    return 0;
}

// 0x80061844
// Fires a spawn at the object's target (+0xA14): direction = facing, but with Z replaced by the
// normalised Z of the vector to the target; then calls fn_80065200 on the spawned object.
void fn_80061844(u8* p, u8* owner) {
    u8* f = (u8*)fn_801B50FC(p);
    f32 dir[3];
    dir[0] = *(f32*)(f + 0xc);
    dir[1] = *(f32*)(f + 0x10);
    dir[2] = *(f32*)(f + 0x14);
    u8* t = *(u8**)(p + 0xa14);
    if (t != NULL) {
        u8* tp = *(u8**)(t + 4);
        u8* pos = *(u8**)(p + 4);
        f32 dx = *(f32*)(tp + 0) - *(f32*)(pos + 0);
        f32 dy = *(f32*)(tp + 4) - *(f32*)(pos + 4);
        f32 dz = *(f32*)(tp + 8) - *(f32*)(pos + 8);
        f32 l2 = (dz * dz) + ((dx * dx) + (dy * dy));
        f32 nx, ny, nz;
        if (l2 == 0.0f) {
            nx = 0.0f;
            ny = 0.0f;
            nz = 0.0f;
        } else {
            f32 inv = 1.0f / sqrtf(l2);  // original: frsqrte with two Newton steps
            nx = dx * inv;
            ny = dy * inv;
            nz = dz * inv;
        }
        (void)nx;
        (void)ny;
        dir[2] = nz;
    }
    u8* s = (u8*)SpawnHelper(owner, *(void**)(p + 0xa14));
    if (s != NULL) fn_80065200(s, p + 0x18, dir, p, 0, 0, 0);
}

// 0x80061A98
// Records the colour tag (4 bytes: fn_801ED624 of the target, or of the found object at +0x200)
// in a 4-entry ring at +0xBBC and, with it, the colour of whichever of the two registered
// objects (+0xBA0 / +0xBA4) is nearer, in the ring at +0xBAC. Ring index at +0xBCC.
void fn_80061A98(u8* p, u8* target, s32 unused) {
    u8 col[4];
    col[0] = 0xFF;
    col[1] = 0;
    *(u16*)(col + 2) = 0;
    u32 key = fn_801ED538(target);
    u8* obj = (u8*)fn_801B7198(&key, 0);
    if (obj != NULL && (*(u32*)(*(u8**)(obj + 4) + 0x1c) & 0x100)) {
        u8 t[4];
        fn_801ED624(t, obj + 0x200);
        col[0] = t[0];
        col[1] = t[1];
        *(u16*)(col + 2) = *(u16*)(t + 2);
    } else if (fn_801ED2FC(target) == 3) {
        u8 t[4];
        fn_801ED624(t, target);
        col[0] = t[0];
        col[1] = t[1];
        *(u16*)(col + 2) = *(u16*)(t + 2);
    }
    if (fn_801C6634(col) != 0) {
        u8* a = (u8*)fn_801C668C(p + 0xba0);
        u8* b = (u8*)fn_801C668C(p + 0xba4);
        if (a != NULL && b != NULL) {
            u8* pos = *(u8**)(p + 4);
            f32 ax = *(f32*)(a + 4) - *(f32*)(pos + 0);
            f32 ay = *(f32*)(a + 8) - *(f32*)(pos + 4);
            f32 az = *(f32*)(a + 0xc) - *(f32*)(pos + 8);
            f32 bx = *(f32*)(b + 4) - *(f32*)(pos + 0);
            f32 by = *(f32*)(b + 8) - *(f32*)(pos + 4);
            f32 bz = *(f32*)(b + 0xc) - *(f32*)(pos + 8);
            f32 da = az * az + (ax * ax + ay * ay);
            f32 db = bz * bz + (bx * bx + by * by);
            u8* slot = p + *(s32*)(p + 0xbcc) * 4;
            if (da < db) {
                *(u8*)(slot + 0xbac) = *(u8*)(p + 0xba0);
                *(u8*)(slot + 0xbad) = *(u8*)(p + 0xba1);
                *(u16*)(slot + 0xbae) = *(u16*)(p + 0xba2);
            } else {
                *(u8*)(slot + 0xbac) = *(u8*)(p + 0xba4);
                *(u8*)(slot + 0xbad) = *(u8*)(p + 0xba5);
                *(u16*)(slot + 0xbae) = *(u16*)(p + 0xba6);
            }
            slot = p + *(s32*)(p + 0xbcc) * 4;
            *(u8*)(slot + 0xbbc) = col[0];
            *(u8*)(slot + 0xbbd) = col[1];
            *(u16*)(slot + 0xbbe) = *(u16*)(col + 2);
            s32 n = *(s32*)(p + 0xbcc) + 1;
            *(s32*)(p + 0xbcc) = n;
            if (n == 4) *(s32*)(p + 0xbcc) = 0;
        }
    }
}

// 0x80061FDC
// Sets the object's behaviour mode (+0xB84) from an event hash; the target-pick event also picks
// a target (fn_80060A28) and caches its position at +0xB88. `flag` goes to +0xB85.
void fn_80061FDC(u8* p, s32 hash, s32 flag) {
    EventSend(gEventMgr, p, p, 0x9C44ADFDu, 0, 0, 1, 0);
    *(u8*)(p + 0xb85) = (u8)flag;
    switch ((u32)hash) {
    case 0x05147C24u: {
        u8* t = (u8*)fn_80060A28(p);
        *(u8**)(p + 0xa14) = t;
        if (t != NULL && EntityVirt4C(t) == 0) {
            u8* t2 = *(u8**)(p + 0xa14);
            f32* v = (*(f32*(**)(u8*, u8*, u8*))(*(u8**)t2 + 0x44))(t2, p + 0x18, p);
            *(f32*)(p + 0xb88) = v[0];
            *(f32*)(p + 0xb8c) = v[1];
            *(f32*)(p + 0xb90) = v[2];
            *(u8*)(p + 0xb84) = 1;
        }
        break;
    }
    case 0x9D9000F4u: *(u8*)(p + 0xb84) = 2; break;
    case 0xE43F4A14u: *(u8*)(p + 0xb84) = 3; break;
    case 0xE607B3F7u: *(u8*)(p + 0xb84) = 4; break;
    case 0x7EAAAE26u: *(u8*)(p + 0xb84) = 5; break;
    case 0xDE451C89u: *(u8*)(p + 0xb84) = 6; break;
    default:
        *(u8*)(p + 0xb84) = 0;
        *(u8*)(p + 0xb85) = 0;
        break;
    }
}

// 0x80062174
// Line-of-sight test from the object to its target (+0xA14): fails if the target is disabled,
// the world ray cast hits something, or any enabled blocker entity (flag 0x20) intersects the ray.
s32 fn_80062174(u8* p) {
    u8* target = *(u8**)(p + 0xa14);
    if (target != NULL) {
        if (EntityVirt4C(target) != 0) return 0;
    } else {
        return 0;
    }
    u8 hit[0x6c];
    memset(hit, 0, 0x6c);
    *(s32*)(hit + 0x54) = 7;
    u8* t1 = *(u8**)(p + 0xa14);
    f32* v = (*(f32*(**)(u8*, u8*, u8*))(*(u8**)t1 + 0x44))(t1, p + 0x18, p);
    if (fn_80138C40(gWorldState, p + 0x18, v, hit, p, 0, 0) != 0) return 0;
    u8* t2 = *(u8**)(p + 0xa14);
    f32* w = (*(f32*(**)(u8*, u8*, u8*))(*(u8**)t2 + 0x44))(t2, p + 0x18, p);
    f32 d[3];
    d[0] = w[0] - *(f32*)(p + 0x18);
    d[1] = w[1] - *(f32*)(p + 0x1c);
    d[2] = w[2] - *(f32*)(p + 0x20);
    for (s32 i = 0; i < 0x20; i++) {
        u8* o = *(u8**)(gGame + 0x140 + i * 4);
        if (o == NULL || EntityVirt4C(o) != 0) continue;
        u8* e = *(u8**)(gGame + 0x140 + i * 4);
        if (!(*(u32*)(*(u8**)(e + 4) + 0x1c) & 0x20)) continue;
        if (e == *(u8**)(p + 0xa14) || e == p) continue;
        if (fn_801C2B40(e + 0x18, p + 0x18, d, *(f32*)(e + 0x14)) != 0) return 0;
    }
    return 1;
}

// 0x80062470
// True when the second target (+0xA18) is in range of the first (+0xA14): inside radius
// (+0xA1C) of target 1, or farther than max(0.5*radius, 0.5 + both body radii) from the owner.
s32 fn_80062470(u8* p) {
    u8* t = *(u8**)(p + 0xa18);
    if (t == NULL) return 0;
    u8* a = *(u8**)(p + 0xa14);
    if (a != NULL) {
        u8* ap = *(u8**)(a + 4);
        u8* tp = *(u8**)(t + 4);
        f32 dx = *(f32*)(ap + 0) - *(f32*)(tp + 0);
        f32 dy = *(f32*)(ap + 4) - *(f32*)(tp + 4);
        f32 dz = *(f32*)(ap + 8) - *(f32*)(tp + 8);
        f32 r = *(f32*)(p + 0xa1c);
        if (dz * dz + (dx * dx + dy * dy) < r * r) return 1;
    }
    u8* tp = *(u8**)(t + 4);
    u8* pp = *(u8**)(p + 4);
    f32 dx = *(f32*)(tp + 0) - *(f32*)(pp + 0);
    f32 dy = *(f32*)(tp + 4) - *(f32*)(pp + 4);
    f32 dz = *(f32*)(tp + 8) - *(f32*)(pp + 8);
    f32 sum = 0.5f + (*(f32*)(p + 0x14) + *(f32*)(t + 0x14));
    f32 r = *(f32*)(p + 0xa1c);
    f32 m1 = 0.5f * r;
    if (m1 <= sum) m1 = sum;
    f32 m2 = 0.5f * r;
    if (m2 <= sum) m2 = sum;
    if (dz * dz + (dx * dx + dy * dy) > m2 * m1) return 1;
    return 0;
}

// 0x80062A98
// True when the facing vector (fn_801B50FC) does not point towards the aim point (dot <= 0).
// The aim point is the vec at +0xB88, or +0xB94 when only flag 0x10 is set; flag 8 selects the
// +0xB88 point unless the owner's zone has +0xC0 set and flag 4 set without flag 1.
s32 fn_80062A98(u8* p) {
    u32 flags = *(u32*)(p + 0xa08);
    s32 mode = 0;
    u8* pos = *(u8**)(p + 4);
    if (flags & 8) {
        u8* zone = *(u8**)(*(u8**)gWorldState + 0x1a4) + *(u16*)(pos + 0x30) * gZoneResourceSize;
        if (*(u8*)(zone + 0xc0) == 0 || !(flags & 4) || (flags & 1)) mode = 1;
    }
    if (mode == 0) {
        if (flags & 0x10) mode = 2;
        else if (flags & 8) mode = 1;
        else return 0;
    }
    s32 off = (mode == 1) ? 0xb88 : 0xb94;
    f32 vx = *(f32*)(p + off + 0) - *(f32*)(pos + 0);
    f32 vy = *(f32*)(p + off + 4) - *(f32*)(pos + 4);
    f32 vz = 0.0f;
    u8* f = (u8*)fn_801B50FC(p);
    f32 fx = *(f32*)(f + 0);
    f32 fy = *(f32*)(f + 4);
    f32 fz = *(f32*)(f + 8);
    f32 dot = (fz * vz) + ((fx * vx) + (fy * vy));
    return !(dot > 0.0f);
}

// 0x800653F4
// Starts animation `idx` of the projectile's anim def table on its render object (creating the
// render handle at +0xA0 on first use). `playing` selects the running/stopped flag state; `restart`
// additionally clears bit 0x10 of the handle flags and kicks fn_8018C3D0.
void fn_800653F4(u8* p, s32 idx, s32 playing, s32 restart) {
    u8* defs = *(u8**)(*(u8**)(p + 0x9c) + 0x48);
    void* hasRender = (*(void*(**)(u8*))(*(u8**)p + 0xc))(p);
    if (hasRender == NULL) {
        char buf[0x50];
        SysSprintf(buf, "ERROR - attempt to animate projectile (%s) without a render obj ...\n", p + 0x108);
        return;
    }
    if (defs == NULL) {
        char buf[0x50];
        SysSprintf(buf, "ERROR - attempt to animate projectile (%s) without anim def table ...\n", p + 0x108);
        return;
    }
    u8* anim = (u8*)fn_801AC184(defs, idx);
    if (anim == NULL) {
        char buf[0x50];
        SysSprintf(buf, "ERROR - unable to find anim [%d] for projectile (%s) ...\n", idx, p + 0x108);
        return;
    }
    if (*(u8**)(p + 0xa0) == NULL) {
        u8* h = (u8*)fn_80007C54();
        *(u8**)(p + 0xa0) = h;
        if (h != NULL) {
            fn_8018C0A8(*(u8**)(p + 0xa0), (*(void*(**)(u8*))(*(u8**)p + 0xc))(p));
            void* a = fn_801AC460(anim, *(u16*)(*(u8**)(p + 4) + 0x32));
            fn_8018B2CC(*(u8**)(p + 0xa0), a, 0, 0, 0.075f, 0.075f);
            fn_8018A948(*(u8**)(p + 0xa0), 0.0f);
        }
    }
    *(u32*)(p + 0x244) &= ~2u;
    fn_801AC51C(anim, *(u8**)(p + 0xa0), p);
    if (playing != 0) {
        u8* q = *(u8**)(p + 4);
        *(u32*)(p + 0x244) |= 1;
        *(u32*)(q + 0x2c) &= ~2u;
        *(s32*)(*(u8**)(p + 0xa0) + 0xf0) = 0;
    } else {
        u8* q = *(u8**)(p + 4);
        *(u32*)(p + 0x244) &= ~1u;
        *(u32*)(q + 0x2c) |= 2;
        *(s32*)(*(u8**)(p + 0xa0) + 0xf0) = 8;
    }
    if (restart != 0) {
        u8* h = *(u8**)(p + 0xa0);
        *(u32*)(h + 0xf0) &= ~0x10u;
        fn_8018C3D0(*(u8**)(p + 0xa0));
    }
}

// 0x80068B50
// Tears down a spawned object's attachments: removes its two possible effect handles (+0x234 /
// +0x238, with the box centred on its position +/- the offset at +0x188), sound, hooks, zone
// registrations; if `notify` is set also calls fn_8012E7B0 for the world.
void fn_80068B50(u8* p, s32 notify) {
    if (*(u8**)(p + 0x234) != NULL) {
        u8* pos = *(u8**)(p + 4);
        f32 hi[3], lo[3];
        hi[0] = *(f32*)(pos + 0) + *(f32*)(p + 0x188);
        hi[1] = *(f32*)(pos + 4) + *(f32*)(p + 0x18c);
        hi[2] = *(f32*)(pos + 8) + *(f32*)(p + 0x190);
        pos = *(u8**)(p + 4);
        lo[0] = *(f32*)(pos + 0) - *(f32*)(p + 0x188);
        lo[1] = *(f32*)(pos + 4) - *(f32*)(p + 0x18c);
        lo[2] = *(f32*)(pos + 8) - *(f32*)(p + 0x190);
        u8* h = *(u8**)(p + 0x234);
        fn_801C4B90(h, hi, lo, 0, 0, *(u8**)(*(u8**)h + 4));
        h = *(u8**)(p + 0x234);
        fn_801C4B90(h, hi, lo, 0, 0, *(u8**)(*(u8**)h + 4));
        fn_801C3E7C(lbl_8047C7AC, *(u8**)(p + 0x234));
        *(u8**)(p + 0x234) = NULL;
    } else if (*(u8**)(p + 0x238) != NULL) {
        u8* pos = *(u8**)(p + 4);
        f32 hi[3], lo[3];
        hi[0] = *(f32*)(pos + 0) + *(f32*)(p + 0x188);
        hi[1] = *(f32*)(pos + 4) + *(f32*)(p + 0x18c);
        hi[2] = *(f32*)(pos + 8) + *(f32*)(p + 0x190);
        pos = *(u8**)(p + 4);
        lo[0] = *(f32*)(pos + 0) - *(f32*)(p + 0x188);
        lo[1] = *(f32*)(pos + 4) - *(f32*)(p + 0x18c);
        lo[2] = *(f32*)(pos + 8) - *(f32*)(p + 0x190);
        fn_801C5CC0(*(u8**)(p + 0x238), hi, lo);
        fn_801C3E88(lbl_8047C7AC, *(u8**)(p + 0x238));
        *(u8**)(p + 0x238) = NULL;
    }
    if (*(s32*)(p + 0x250) != -1) {
        fn_8020A398(*(s32*)(p + 0x250));
        *(s32*)(p + 0x250) = -1;
    }
    fn_80062F58(p);
    fn_801E839C(p + 0xfc);
    u8* q = *(u8**)(p + 0x228);
    if (q != NULL) {
        u32 key = *(u32*)(q + 0x104);
        if (key != 0) {
            u8* w = *(u8**)(p + 0x254);
            if (w != NULL) {
                // r6 (4th arg) was not set before this call in the original: caller leftover
                fn_801DB398(*(u8**)(*(u8**)gWorldState + 0x1a4) + 0xe4, key, w);
                *(u8**)(p + 0x254) = NULL;
            }
            *(s32*)(p + 0x90) = 0;
        }
    }
    u32 flags = *(u32*)(p + 0xb8);
    if ((flags & 1) && !(flags & 2)) {
        fn_801DB398(*(u8**)(*(u8**)gWorldState + 0x1a4) + 0xe4, 0xBE8520ABu, p);
        *(u32*)(p + 0xb8) |= 2;
        if (notify != 0) fn_8012E7B0(gWorldState, p, 0);
    }
    u8* hook = *(u8**)(p + 0x25c);
    if (hook != NULL) {
        fn_800634D0(hook);
        fn_80063520(*(u8**)(p + 0x25c));
        *(u8**)(p + 0x25c) = NULL;
    }
    *(s32*)(p + 0x23c) = 0;
    *(s32*)(p + 0x248) = 0;
}

// 0x80069180
// Resets the object's runtime fields from its def (+0x9C), applies the def's flags/opacity,
// fires the def's and the object's trigger actions, then runs fn_80062DC4(p, 0, a2).
s32 fn_80069180(u8* p, s32 unused, s32 a2) {
    u8* def = *(u8**)(p + 0x9c);
    *(s32*)(p + 0x198) = 0;
    *(f32*)(p + 0x168) = 0.0f;
    *(f32*)(p + 0x16c) = 0.0f;
    *(f32*)(p + 0x170) = 0.0f;
    *(f32*)(p + 0x174) = 0.0f;
    *(f32*)(p + 0x19c) = 0.0f;
    *(f32*)(p + 0x1b0) = 0.0f;
    *(f32*)(p + 0x1b4) = 0.0f;
    *(f32*)(p + 0x1b8) = 0.0f;
    *(f32*)(p + 0x1bc) = 0.0f;
    *(f32*)(p + 0x1c0) = 0.0f;
    if (def != NULL) {
        f32 v = *(f32*)(def + 0x24);
        *(f32*)(p + 0x158) = v;
        if (a2 == 0) *(f32*)(*(u8**)(p + 4) + 0x24) = v;
        def = *(u8**)(p + 0x9c);
        *(s32*)(p + 0x15c) = *(s32*)(def + 0xbc);
        if (*(u8*)(def + 0x1c) != 0 && (*(s32 (**)(u8*))(*(u8**)p + 0x10))(p) != 0 &&
            !(*(u32*)(p + 0x15c) & 1)) {
            u8* st = *(u8**)(p + 4);
            *(u32*)(st + 0x20) |= 0x8000;
            st = *(u8**)(p + 4);
            *(u32*)(st + 0x20) |= 0x4000;
        }
        f32 alpha = 0.0f;
        if (*(s32*)(p + 0xa0) != 0) alpha = (f32)*(u8*)(*(u8**)(p + 4) + 0x38) / 255.0f;
        fn_800699E4(p, *(u8*)(*(u8**)(p + 4) + 0x35), alpha);
        u8* d2 = *(u8**)(p + 0x9c);
        if (d2 != NULL && ((*(u32*)(d2 + 0x98) >> 27) & 0xF)) TriggerDoAction(d2 + 0x98, p, NULL);
        if ((*(u32*)(p + 0x68) >> 27) & 0xF) TriggerDoAction(p + 0x68, p, NULL);
    }
    if (*(u32*)(*(u8**)(p + 0x9c) + 8) == 0x1AC481C5u) {
        lbl_805FB1D8[0] = 0;
        lbl_805FB1E0[0] = 0;
        lbl_805FB1D8[1] = 0;
        lbl_805FB1E0[1] = 0;
    }
    fn_80062DC4(p, 0, a2);
    return 1; // asm returns r3 as left by the call chain (ends in fn_801B5BD8, which returns 1)
}

// 0x800696C4
// Despawn/disable hook: clears the motion fields, fires the def's and object's trigger actions
// and, for mode 2, the "gone" actions; for any mode releases the attachment at +0x160 and
// clears the two cached effect handles of the 0x1AC481C5 object type.
void fn_800696C4(u8* p, s32 mode) {
    if (*(s32*)(p + 4) == 0) return;
    *(f32*)(p + 0x168) = 0.0f;
    *(f32*)(p + 0x16c) = 0.0f;
    *(f32*)(p + 0x170) = 0.0f;
    *(f32*)(p + 0x174) = 0.0f;
    fn_80062F58(p);
    u8* def = *(u8**)(p + 0x9c);
    if (def != NULL && ((*(u32*)(def + 0x84) >> 27) & 0xF)) TriggerDoAction(def + 0x84, p, NULL);
    if ((*(u32*)(p + 0x54) >> 27) & 0xF) TriggerDoAction(p + 0x54, p, NULL);
    if (mode != 0 && mode == 2) {
        if (!(*(u32*)(*(u8**)(p + 4) + 0x20) & 0x08000000)) {
            fn_801ED130(p + 0x94);
            u8* d2 = *(u8**)(p + 0x9c);
            if (d2 != NULL && ((*(u32*)(d2 + 0x78) >> 27) & 0xF)) TriggerDoAction(d2 + 0x78, p, NULL);
            if ((*(u32*)(p + 0x48) >> 27) & 0xF) TriggerDoAction(p + 0x48, p, NULL);
            *(u32*)(*(u8**)(p + 4) + 0x20) |= 0x08000000;
        }
        fn_801E839C(p + 0xfc);
        *(u8*)(*(u8**)(p + 4) + 0x35) = 0;
        *(u8*)(*(u8**)(p + 4) + 0x36) = 0;
        if (fn_801F5220() != 0 || *(u32*)(p + 0xec) == 0x6C938971u) {
            u8* st = *(u8**)(p + 4);
            if (*(s32*)(st + 0x28) > 0) *(u32*)(st + 0x20) |= 0x20000;
        }
    }
    if (mode != 0) {
        s32 saved = lbl_805FB9A4;
        lbl_805FB9A4 = 0;
        u8* o = *(u8**)(p + 0x160);
        if (o != NULL) (*(void (**)(u8*))(*(u8**)o + 0x58))(o);
        *(u8**)(p + 0x160) = NULL;
        *(s32*)(p + 0x164) = -1;
        lbl_805FB9A4 = saved;
    }
    if (*(u32*)(*(u8**)(p + 0x9c) + 8) == 0x1AC481C5u && mode != 0) {
        for (s32 i = 0; i < 2; i++) {
            if (lbl_805FB1E0[i] != 0) {
                fn_801C3E88(lbl_8047C7AC, (void*)(intptr_t)lbl_805FB1E0[i]);
                lbl_805FB1E0[i] = 0;
            }
            lbl_805FB1D8[i] = 0;
        }
    }
}

// 0x8006B808
// Spawns the child object for slot `spec` of the 0x37645F52-type object `obj` (default: the
// current slot's +0xC4), replacing the previous child (+0x160) and aligning its orientation.
void fn_8006B808(u8* p, u8* obj, u8* spec) {
    u8* slot = *(u8**)(*(u8**)(p + 0x1a0) + *(u8*)(*(u8**)(p + 4) + 0x35) * 4);
    if (obj == NULL) obj = *(u8**)(slot + 0xc4);
    s32 idx;
    if (*(u16*)spec & 1) idx = *(u8*)(spec + 2);
    else idx = -1;
    if (idx < 0) idx = fn_801EC880(p + 0x94, *(s32*)(slot + 0xcc));
    if (obj == NULL) return;
    if (*(u32*)obj != 0x37645F52u) return;
    if (idx < 0) return;
    u8* entry = *(u8**)(p + 0x98) + idx * 0xc8 + 0x5c;
    if (*(u8**)(obj + 0x120) != NULL) obj = *(u8**)(obj + 0x120);
    u8* child = (u8*)SpawnHelper(obj, 0);
    if (child == NULL) return;
    u8* old = *(u8**)(p + 0x160);
    if (old != NULL) (*(void (**)(u8*))(*(u8**)old + 0x58))(old);
    *(u8**)(p + 0x160) = child;
    *(s32*)(p + 0x164) = idx;
    f32 zero[3];
    zero[2] = 0.0f;
    zero[1] = 0.0f;
    zero[0] = 0.0f;
    fn_80065200(*(u8**)(p + 0x160), entry, zero, NULL, 0, 0x80, 0x80);
    QuatFromMtx((Quat*)(*(u8**)(*(u8**)(p + 0x160) + 4) + 0xc),
                (const f32*)(*(u8**)(p + 0x98) + *(s32*)(p + 0x164) * 0xc8 + 0x38));
}

// 0x8006E9D8
// SetTimeoutFromProperty: picks a timeout for the FSM at +0xA4's state (+0x4C/+0x50) from a
// property hash: fixed (+0x160) or random within value +/- spread (+0x158/+0x170 or +0x15C/+0x174).
void fn_8006E9D8(u8* p, s32 hash) {
    f32 t = -1.0f;
    switch ((u32)hash) {
    case 0xEF6D2B59u: {
        f32 base = *(f32*)(p + 0x15c);
        f32 spread = fabsf(*(f32*)(p + 0x174));
        f32 lo = base - spread;
        if (!(lo > 0.0f)) lo = 0.0f;
        f32 hi = base + spread;
        if (!(hi > lo)) hi = lo;
        t = lo + ((hi - lo) * (2.3283064e-10f * (f32)RandomU32()));
        break;
    }
    case 0x973C055Bu: {
        f32 base = *(f32*)(p + 0x158);
        f32 spread = fabsf(*(f32*)(p + 0x170));
        f32 lo = base - spread;
        if (!(lo > 0.0f)) lo = 0.0f;
        f32 hi = base + spread;
        if (!(hi > lo)) hi = lo;
        t = lo + ((hi - lo) * (2.3283064e-10f * (f32)RandomU32()));
        break;
    }
    case 0xDC4DDAD6u:
        t = *(f32*)(p + 0x160);
        break;
    default:
        GameLog(4, 6, "%s.SetTimeoutFromProperty(): property not found\n", fn_801B6E74(p));
        break;
    }
    if (t >= 0.0f) {
        u8** fsm = *(u8***)(p + 0xa4);
        if (fsm != NULL) {
            u8* st = *fsm;
            if (st != NULL) {
                *(f32*)(st + 0x4c) = t;
                *(f32*)(st + 0x50) = t;
                return;
            }
        }
        GameLog(4, 6, "%s.SetTimeoutFromProperty(): not FSM\n", fn_801B6E74(p));
    }
}

// 0x8012649C
// Camera/pose helper: takes the two vectors of the current slot (lbl_805FB2E8 +0x118/+0x11C entries,
// +8..+0x10), lets the entity from fn_800E60BC offset the second by its virtual +0xC vector, then
// hands the pair to the lbl_803C4298 object (virtual +0x2C, +0xC(6), +0x30(1.5)) and sets flag +0x10C.
void fn_8012649C(void) {
    u8* g = lbl_805FB2E8;
    f32 a[3], b[3];
    u8* e0 = *(u8**)(g + *(s32*)(g + 0x110) * 8 + 0x118);
    a[0] = *(f32*)(e0 + 0x8);
    a[1] = *(f32*)(e0 + 0xc);
    a[2] = *(f32*)(e0 + 0x10);
    u8* e1 = *(u8**)(g + *(s32*)(g + 0x110) * 8 + 0x11c);
    b[0] = *(f32*)(e1 + 0x8);
    b[1] = *(f32*)(e1 + 0xc);
    b[2] = *(f32*)(e1 + 0x10);
    u8* o = (u8*)fn_800E60BC();
    if (o != NULL) {
        u8 saved = *(u8*)(o + 0x54);
        f32 off[3];
        *(u8*)(o + 0x54) = 1;
        (*(void (**)(u8*, f32*, f32*, f32))(*(u8**)o + 0xc))(o, a, off, 0.0f);
        b[0] = a[0] + off[0];
        b[1] = a[1] + off[1];
        b[2] = a[2] + off[2];
        *(u8*)(o + 0x54) = saved;
    }
    u8* q = *(u8**)(lbl_803C4298 + 4);
    (*(void (**)(u8*, f32*, f32*))(*(u8**)q + 0x2c))(q, a, b);
    q = *(u8**)(lbl_803C4298 + 4);
    (*(void (**)(u8*, s32))(*(u8**)q + 0xc))(q, 6);
    q = *(u8**)(lbl_803C4298 + 4);
    (*(void (**)(u8*, f32))(*(u8**)q + 0x30))(q, 1.5f);
    *(u8*)(lbl_805FB2E8 + 0x10c) = 1;
}

// 0x801BBB68
// Sends event `id` (a,b = its two arguments; for the special 0x04D27185 event the arguments are
// (a -> +0x24, b becomes the id)) to the current node of the manager's table: looks up a handler
// in the node, then among the table's entries, and if found and accepted notifies fn_801BBC9C.
void fn_801BBB68(void* mgr, s32 id, int a, int b) {
    u8* m = (u8*)mgr;
    if ((u32)id == 0x04D27185u) {
        *(s32*)(m + 0x24) = a;
        id = b;
        *(s32*)(m + 0x14) = 0;
        *(s32*)(m + 0x18) = 0;
    } else {
        *(s32*)(m + 0x14) = a;
        *(s32*)(m + 0x18) = b;
    }
    s32 idx = *(s32*)(m + 4);
    u8* cur = NULL;
    if (idx > -1) cur = **(u8***)m + idx * 0x30;
    u8* found = fn_801BC1C0(cur, id);
    if (found == NULL) {
        idx = *(s32*)(m + 4);
        u8* node = NULL;
        if (idx > -1) node = **(u8***)m + idx * 0x30;
        s32 sub = 0;
        if (node != NULL) sub = *(s32*)(node + 4);
        found = fn_801BC6F0(*(u8**)m, sub, id);
    }
    if (found != NULL && fn_801BC41C(found, m) != 0) {
        idx = *(s32*)(m + 4);
        u8* node = NULL;
        if (idx > -1) node = **(u8***)m + idx * 0x30;
        fn_801BBC9C(m, node, found);
    }
}

}  // extern "C"
