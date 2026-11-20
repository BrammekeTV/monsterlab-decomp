// Miscellaneous functions from the sixth "missing" draft batch: combat HUD selection, prop state
// step and effects, trigger camera/zone events, footstep and music sounds, save free-space checks,
// UI cursor flush, texture loading, render default state, stick and timer helpers.
#include <string.h>

#include "types.h"
#include "revolution.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

struct ScopedLock { u8 pad[4]; };   // as in props.cpp

typedef u8*  (*GetCCFn)(void*);                                  // object vtable +0xCC
typedef void (*UIVFn)(void*);                                    // element vtable (+0x138) slot
typedef void (*UIVisFn)(void*, s32);                             // element vtable (+0x138): 0x2C show, 0x30 hide
typedef void (*UIColorFn)(void*, s32, f32, f32, f32, f32);       // element vtable (+0x138) +0x2C with a mode and floats
typedef s32  (*ShapeFn)(u8*, u8*);

extern "C" {
void   CharacterStartReaction(void* ch);                         // 0x8004E3D0
void   CombatHudRefreshActions(void);                            // 0x800476D8
void*  EventPost(s32 id, void* sender);                          // 0x801F3918
void   EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void   EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);   // 0x801B9F98
void   LimbPlayReaction(void* limb, u8* reaction, BOOL special); // 0x8004F398
s32    NANDCheck(u32 fsBlock, u32 inode, u32* answer);           // 0x80268194
void   ReactionBuildRecover(u8* out, void* fighter);             // 0x802159C4
void   ScopedLockCtor(ScopedLock* l);                            // 0x801F5384
void   ScopedLockDtor(ScopedLock* l, s32 flags);                 // 0x801F53B0
void*  ScreenSetFindElement(void* set, u32 hash);                // 0x80198F18
void   ScreenSetsActivate(void* sets, u32 hash);                 // 0x8018D70C
void*  ScreenSetsCurrent(void* sets);                            // 0x8018D1E4
int    SysSprintf(char* buf, const char* fmt, ...);              // 0x80142EFC
s32 TriggerDoAction(void* action, void* a1, void* a2);        // 0x801E0054
void   UIPlayAnim(void* el, u32 anim);                           // 0x8019BE40
void   Vec3Assign(Vec3* out, const Vec3* v);                     // 0x80053354
void   Vec3SetZero(Vec3* out);                                   // 0x800531A8
s32    WorldGetPhase(void* world);                               // 0x800D83A8
void   PropPlayFx(void* prop, u32 key);                         // 0x801254F8
BOOL   SpawnEffectAtPos(u8* mgr, u8* effect, const f32* pos, u8** owner);   // 0x801C81D8
s32    PlaySound(u32 hash, void* owner, s32 b, s32 c, s32 d, s32 e);   // 0x80209E70 (owner: entity or 0)
s32    PlaySoundAt(void* sound, int a, const void* pos, int b);  // 0x80209ABC
f32    cosf(f32 x);
f32    sinf(f32 x);

u32    fn_8003F858(void);                                        // 0x8003F858
s32    fn_8003F8D4(void);                                        // 0x8003F8D4
u32    fn_80044C0C(void);                                        // 0x80044C0C
void   fn_80046F5C(void);                                        // 0x80046F5C
void   fn_8004739C(s32 action);                                  // 0x8004739C
s32    fn_8004E2A4(u8* fighter, s32 slot);                       // 0x8004E2A4
void   fn_8004F310(void* limb, s32 idx, s32 a, s32 b);           // 0x8004F310
void   fn_800699E4(u8* prop, u32 state, f32 t);                  // 0x800699E4
void   fn_80069EEC(u8* prop);                                    // 0x80069EEC
s32    fn_800D5800(u8* p);                                       // 0x800D5800
void   fn_8010AAE0(void);                                        // 0x8010AAE0
void   fn_8010AB0C(s32 a, s32 b);                                // 0x8010AB0C
void   fn_8010AB70(void);                                        // 0x8010AB70
void   fn_801225F4(u8* prop, u8* data, s32 a, s32 b);            // 0x801225F4
s32    fn_80124824(u8* collider);                                // 0x80124824 (adds the collider volume to gDebrisVolumeUnit)
BOOL   DebrisSpawnForCollider(u8* collider);                     // 0x801249C4
s32    fn_8012EB74(u8* world, u32 arg, s32 a, s32 b, s32 c, s32 d);   // 0x8012EB74
void   fn_80148770(s32 n);                                       // 0x80148770
void   fn_8014879C(f32* out);                                    // 0x8014879C
void   fn_801487B4(f32* out);                                    // 0x801487B4
void   fn_801487CC(const f32* a, const f32* b, const f32* c, const f32* d);   // 0x801487CC
f32*   fn_80148D70(f32* dst, const f32* src);                    // 0x80148D70 (copies 4 floats, returns dst)
void   fn_80148D94(const f32* v);                                // 0x80148D94
void   fn_80149094(Vec3* out);                                   // 0x80149094
void   fn_801490AC(s32 idx, const Vec3* a, const Vec3* b, f32 x, f32 y);   // 0x801490AC
void   fn_8014930C(s32 idx, const f32* color);                   // 0x8014930C
void   fn_80149428(s32 idx, const Vec3* v);                      // 0x80149428
void   fn_80149464(s32 idx, s32 a, s32 b);                       // 0x80149464
void   fn_80149468(s32 idx);                                     // 0x80149468
void   fn_801494B4(void);                                        // 0x801494B4
void   fn_8014950C(s32 a, s32 b);                                // 0x8014950C
void   fn_801514F0(void);                                        // 0x801514F0
void   fn_8015150C(const f32* v, f32 a, f32 b);                  // 0x8015150C
void   fn_80149540(void);                                        // 0x80149540
void   fn_80149594(void);                                        // 0x80149594
void   fn_8014970C(s32 a, s32 b);                         // 0x8014970C
void   fn_80149744(s32 a);                                       // 0x80149744
void   fn_8014977C(f32 v);                                       // 0x8014977C
void   fn_80149780(void);                                        // 0x80149780
void   fn_801497AC(void);                                        // 0x801497AC
void   fn_801497B0(f32 v);                                       // 0x801497B0
void   fn_80154294(void);                                        // 0x80154294
void   fn_80154318(void);                                        // 0x80154318
void   fn_8015428C(s32 a);                                       // 0x8015428C
void   fn_80154290(s32 a);                                       // 0x80154290
void   fn_80154974(void);                                        // 0x80154974
void   fn_80155D70(void);                                        // 0x80155D70
void   fn_801566E8(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);    // 0x801566E8
void   fn_80156894(s32 i);                                       // 0x80156894
u8*    fn_8016A08C(const char* name);                            // 0x8016A08C (texture cache lookup)
void   fn_8016964C(u8* mgr, u8* tex);                            // 0x8016964C
s32    fn_801478C8(u8* table, s32 idx);                          // 0x801478C8
void   fn_8018D2B8(const char* fmt, ...);                        // 0x8018D2B8 (debug log)
void   fn_8018D370(const char* fmt, ...);                        // 0x8018D370 (debug log)
void   fn_8019AA10(u8* widget, u32 a);                           // 0x8019AA10
s32 fn_8019BC80(u8* arg0, u32 arg1);                          // 0x8019BC80
u8*    fn_8019BF64(u8* container, u32 hash);                     // 0x8019BF64
u8*    fn_801B3308(void);                                        // 0x801B3308
void   fn_801B3378(u8* p, u32 key);                              // 0x801B3378
void   fn_801B154C(u8* group, u32 key, ShapeFn fn, u8* arg);     // 0x801B154C
void   fn_801BF398(u8* list, u8* item);                          // 0x801BF398
u8* fn_801C7B20(u8* fx, u8* desc, const f32* pos, u8* mgr);   // 0x801C7B20
void   fn_801C849C(u8* mgr, u8* item);                           // 0x801C849C
u8*    fn_801D8D80(char* path, const char* name);                // 0x801D8D80 (texture file load)
void*  fn_801D40E0(s32 n);                                       // 0x801D40E0
const char* fn_801EB708(u32 hash);                               // 0x801EB708 (hash -> name for logs)
void*  fn_801EB94C(void* p);                                     // 0x801EB94C
u32    fn_801EC880(u8* p, u32 hash);                             // 0x801EC880
void   fn_801EC8C4(u8* p, u32 a, u32 b);                         // 0x801EC8C4
void   fn_801ED130(void* p);                                     // 0x801ED130
void   fn_801FD730(u8* audio);                                   // 0x801FD730
void   fn_801FFC04(u8* audio, f32 v);                            // 0x801FFC04
void*  fn_80209814(void* v);                                     // 0x80209814
void   fn_8020A398(s32 handle);                                  // 0x8020A398
s32    fn_80212E28(u8** slot);                                   // 0x80212E28
s32    fn_80216700(GameDb* db, s32 key, s32 index);              // 0x80216700
s32    fn_80216754(GameDb* db, u32 key);                         // 0x80216754
s32    fn_8026829C(u32* freeBlocks, u32* freeInodes);            // 0x8026829C
}

// Defined in save.cpp with C++ linkage.
extern "C++" {
BOOL   SaveFileExists(const char* dir, const char* file);        // 0x80011960
}

extern u8   gAudio[];                    // audio manager
extern u8*  gBattle;                     // 0x805FBBB8
extern f32  gDebrisVolume;               // 0x805FB2E0
extern f32  gDebrisVolumeUnit;           // 0x805FB2DC
extern u8*  gDebrisEffect;               // 0x805FB2E4
extern "C++" void* gEventMgr;            // 0x805FB6C8
extern u8   gGame[];                     // 0x803B53A0
extern "C++" void* gScreenSets;          // 0x805FB654
extern s32  gSelectedPart;               // 0x805F8650 (combat_hud.cpp)
extern s32  gHighlightedAction;          // 0x805F8654 (combat_hud.cpp)
extern s32  gPartSlotState[];            // 0x803C52E0 (combat_hud.cpp)
extern u8   gWorldState[];               // 0x803E6D00 (first word points to the world block)
extern const char lbl_80381318[];        // 0x80381318 (log: cursor not defined)
extern const char lbl_80381868[];        // 0x80381868 (UI script log strings)
extern const char lbl_803818A8[];        // 0x803818A8 (log: no active screen set)
extern const char lbl_80381AA0[];        // 0x80381AA0 (log: element not found)
extern const u32  lbl_80358A08[];        // 0x80358A08 (HUD animation hashes)
extern const u32  lbl_805F85E8[];        // 0x805F85E8 (UI events sent while an action is not ready)
extern const u32  lbl_805F8630[];        // 0x805F8630 (HUD element hashes)
extern const char lbl_805F8FD4[];        // 0x805F8FD4 (save directory string)
extern const char* lbl_805F8FD0;         // 0x805F8FD0 (save file name)
extern f32  lbl_805F9970;                // 0x805F9970
extern f32  lbl_805F9974;                // 0x805F9974
extern f32  lbl_805F9978;                // 0x805F9978 (last clamped music level)
extern f32  lbl_805F997C;                // 0x805F997C
extern u8   lbl_805FB09C[];              // 0x805FB09C (per action: ready flag, sActionReady in combat_hud.cpp)
extern u8   lbl_805FB0A8[];              // 0x805FB0A8
extern u8   lbl_805FB0A9[];              // 0x805FB0A9
extern s32  lbl_805FB0B0[];              // 0x805FB0B0
extern u8   lbl_805FB0BE[];              // 0x805FB0BE
extern u8   lbl_805FB2D5;                // 0x805FB2D5 (save error flag)
extern s32  lbl_805FB2D8;                // 0x805FB2D8
extern u8*  lbl_805FB490;                // 0x805FB490 (current register table)
extern u8*  lbl_805FB5A8;                // 0x805FB5A8 (texture manager)
extern void* lbl_805FBBAC;               // 0x805FBBAC (sound context)
extern u8   lbl_805FBBB3;                // 0x805FBBB3 (music enabled flag)
extern u8   lbl_8047C760[];              // 0x8047C760 (effect manager)
extern u8   lbl_8047C790[];              // 0x8047C790

#define HUD_COMBAT 0x1B66AC94   // "UI_MSA_Combat_HUD"

static const u32 kHashBase = 0xF62113D5u;   // "Base"
static const u32 kHashZone = 0x967E2DB3u;   // "zONE"

// Creates the sound context on first use.
static void EnsureSoundContext(void) {
    if (lbl_805FBBAC == NULL) {
        void* p = fn_801D40E0(1);
        if (p != NULL) p = fn_80209814(p);
        lbl_805FBBAC = p;
    }
}

extern "C" {

// 0x80046BA4
// Shows the combat selection HUD: re-selects the HUD screen, plays the cursor animations, restarts
// the reactions of every fighter's limbs and starts the camera animation.
void ShowSelectionHUD(void) {
    fn_80046F5C();
    if (lbl_805FB0A8[0] != 0) {
        lbl_805FB0A9[0] = 1;
        return;
    }
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    if (*(u32*)set != HUD_COMBAT) return;

    UIEvent_SendToAllScreens(0x94B21702);
    ScreenSetsActivate(gScreenSets, 0x7A1060A7);
    for (s32 i = 0; i < 2; i++) {
        if (lbl_805FB09C[i] == 0) UIEvent_SendToAllScreens(lbl_805F85E8[i]);
    }

    u8* cur = (u8*)ScreenSetsCurrent(gScreenSets);
    if (cur != NULL && *(u32*)cur == HUD_COMBAT) {
        void* el = ScreenSetFindElement(cur, lbl_805F8630[0]);
        if (el != NULL && lbl_805FB0B0[0] != 1) {
            lbl_805FB0B0[0] = 1;
            UIPlayAnim(el, lbl_80358A08[1]);
        }
    }
    cur = (u8*)ScreenSetsCurrent(gScreenSets);
    if (cur != NULL && *(u32*)cur == HUD_COMBAT) {
        void* el = ScreenSetFindElement(cur, lbl_805F8630[1]);
        if (el != NULL && lbl_805FB0B0[1] != 1) {
            lbl_805FB0B0[1] = 1;
            UIPlayAnim(el, lbl_80358A08[3]);
        }
    }

    u8* el = (u8*)ScreenSetFindElement(set, 0x7A1060A7);
    if (el != NULL) (*(UIVisFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);

    for (s32 f = 0; f < 2; f++) {
        u8* fighter = *(u8**)(gBattle + f * 4 + 0xA00);
        u8 reactions[5 * 0x100];
        ReactionBuildRecover(reactions, fighter);
        for (s32 j = 0; j < 5; j++) {
            u8* limb = *(u8**)(fighter + j * 4);
            if (limb != NULL) {
                u8* obj = *(u8**)(limb + 0x20);
                if (obj != NULL) obj -= 0x264;
                LimbPlayReaction(obj, reactions + j * 0x100, 1);
            }
        }
        u8* ch = *(u8**)(fighter + 0x1C);
        if (ch != NULL) ch -= 0xC48;
        CharacterStartReaction(ch);
    }

    if (fn_8003F8D4() != 0) {
        PlayBossCameraAnimation(fn_8003F858());
    } else {
        PlayCameraAnimation(fn_8003F858());
    }
    SendNISSignal(0x10A8EECC);
    lbl_805FB0BE[0] = 1;
}

// 0x80046FE4
// Highlights an action (-1 clears it). For a real action it looks up the action type of the selected
// part's action (the second action when a0 is 1) and shows the first-use tutorial prompts: type 1
// disables the HUD input for the NIS unless already seen, type 2 sends its own NIS signal.
void SetActiveAction(s32 a0) {
    gHighlightedAction = a0;
    if (a0 == -1) {
        gPartSlotState[5] = -1;
        gPartSlotState[6] = -1;
        gPartSlotState[7] = -1;
        gPartSlotState[8] = -1;
        gPartSlotState[9] = -1;
        CombatHudRefreshActions();
        return;
    }
    fn_8004739C(a0);
    CombatHudRefreshActions();

    PartDef def;
    PartDbGetDef(&def, gGameDb, ***(s32***)(*(u8**)(gBattle + 0xA00) + gSelectedPart * 4));
    u8* actions = *(u8**)(gGameDb->data + 0x430);
    s32 actionIndex = def.action1;
    if (a0 == 1) actionIndex = def.action2;
    s32 type = *(s32*)(actions + actionIndex * 0x14 + 4);
    u8* types = *(u8**)(gGameDb->data + 0x08);
    s32 typeKey = *(s32*)(types + type * 0x24 + 8);

    if (typeKey == 1 && !CheckGameProgressBitFlag(0xD)) {
        BOOL tutorial = FALSE;
        if (gBattle != NULL) tutorial = (*(s32*)(gBattle + 0xAD0) == 4);
        if (!tutorial && !IsMultiPlayer()) {
            CombatHUDDisableInput(0xADBF3357);
            SendNISSignal(0xBCF84BFA);
        }
    }
    if (typeKey == 2) {
        SendNISSignal(0x898BBA7B);
    }
}

// 0x80069FDC
// Advances a prop to its next state. Plays the current state's effect and sound, releases its emitter
// and handle, then (when a next state exists) fires the state actions, sets the new state and adds the
// state's size. Returns 1 when it advanced, 0 when there was no next state (the prop is finished).
BOOL PropIncrementState(void* prop) {
    u8* p = (u8*)prop;
    ScopedLock lock;
    ScopedLockCtor(&lock);
    u8* def = *(u8**)(p + 0x9C);
    if (def == NULL) {
        ScopedLockDtor(&lock, -1);
        return 0;
    }

    u32 fx = *(u32*)(def + 0xB4);
    if (fx != 0) PropPlayFx(p, fx);
    EnsureSoundContext();
    PlaySoundAt(*(void**)(def + 0xB8), 0, *(u8**)(p + 4), 0);
    s8 effectDuration = *(s8*)(def + 0xC0);
    if (effectDuration > 0) fn_801225F4(p, *(u8**)(p + 4), 0, effectDuration);
    u8* emitter = *(u8**)(p + 0xA8);
    if (emitter != NULL) {
        fn_801C849C(lbl_8047C760, emitter);
        *(u8**)(p + 0xA8) = NULL;
    }
    fn_801ED130(p + 0x94);
    s32 handle = *(s32*)(p + 0xAC);
    if (handle != -1) {
        fn_8020A398(handle);
        *(s32*)(p + 0xAC) = -1;
    }

    u8* state = *(u8**)(p + 4);
    if ((s32)(*(u8*)(state + 0x35) + 1) >= *(s32*)(p + 0x1A4)) {
        fn_80069EEC(p);
        ScopedLockDtor(&lock, -1);
        return 0;
    }

    u8* d = *(u8**)(p + 0x9C);
    if (d != NULL && ((*(u32*)(d + 0x94) >> 27) & 0xF)) TriggerDoAction(d + 0x94, p, NULL);
    if ((*(u32*)(p + 0x64) >> 27) & 0xF) TriggerDoAction(p + 0x64, p, NULL);
    f32 t = 0.0f;
    if (*(s32*)(p + 0xA0) != 0) t = (f32)*(u8*)(*(u8**)(p + 4) + 0x38) / 255.0f;
    fn_800699E4(p, *(u8*)(*(u8**)(p + 4) + 0x35) + 1, t);
    d = *(u8**)(p + 0x9C);
    if (d != NULL && ((*(u32*)(d + 0x98) >> 27) & 0xF)) TriggerDoAction(d + 0x98, p, NULL);
    if ((*(u32*)(p + 0x68) >> 27) & 0xF) TriggerDoAction(p + 0x68, p, NULL);
    u8* cur = *(u8**)(p + 4);
    *(f32*)(cur + 0x24) = *(f32*)(cur + 0x24) + *(f32*)(*(u8**)(p + 0x9C) + 0x24);
    *(f32*)(p + 0x158) = *(f32*)(*(u8**)(p + 0x9C) + 0x24);
    ScopedLockDtor(&lock, -1);
    return 1;
}

// 0x8006D3C8
// Camera trigger end. With immediate set it only marks the trigger ended (flags 0x600) and clears the
// camera's active bit. Otherwise it sets the bit and, if an end was pending (flag 0x200), runs the end
// actions for every recorded entity, posts the exit event and releases the effect and handles.
void TriggerEndCamera(u8* trigger, s32 immediate) {
    u8* t = trigger;
    if (immediate != 0) {
        u8* data = *(u8**)(t + 4);
        *(u32*)(t + 0x1B0) |= 0x600;
        *(u32*)(data + 0x2C) &= ~1u;
        return;
    }
    u8* data = *(u8**)(t + 4);
    *(u32*)(data + 0x2C) |= 1;
    u32 flags = *(u32*)(t + 0x1B0);
    if (!(flags & 0x200)) return;

    u32 cleared = flags & ~0x200u;
    *(u32*)(t + 0x1B0) = cleared;
    if (!(flags & 8)) *(u32*)(t + 0x1B0) = cleared & ~0x400u;

    for (s32 i = 0; i < 5; i++) {
        u8* entity = *(u8**)(t + 0x1D4 + i * 8);
        if (entity != NULL) {
            *(u8**)(t + 0x1D4 + i * 8) = NULL;
            u8* d = *(u8**)(t + 0x9C);
            if (d != NULL && ((*(u32*)(d + 0xA4) >> 27) & 0xF)) TriggerDoAction(d + 0xA4, entity, t);
            if ((*(u32*)(t + 0x74) >> 27) & 0xF) TriggerDoAction(t + 0x74, entity, t);
            EventSend(gEventMgr, t, t, 0xA29C69BA /* "Exit" */, 0, 0, 1, 0);
        }
    }
    if (!(*(u32*)(*(u8**)(t + 4) + 0x20) & 0x04000000)) EventPost(0xC, t);
    if (*(u8**)(t + 0x178) != NULL) {
        ScopedLock lock;
        ScopedLockCtor(&lock);
        fn_801C849C(lbl_8047C760, *(u8**)(t + 0x178));
        *(u8**)(t + 0x178) = NULL;
        ScopedLockDtor(&lock, -1);
    }
    for (s32 i = 0; i < 2; i++) {
        u8* item = *(u8**)(t + 0x17C + i * 4);
        if (item != NULL) {
            fn_801BF398(lbl_8047C790, item);
            *(u8**)(t + 0x17C + i * 4) = NULL;
        }
    }
}

// 0x8006EBC4
// Sends an event to the four zone targets of a trigger (slots at +0x184). The zone hash goes to the
// world state, everything else through the event manager.
void TriggerSendToZone(void* trigger, u32 event) {
    u8* t = (u8*)trigger;
    if (event == 0) return;
    for (s32 i = 0; i < 4; i++) {
        u32 target = *(u32*)(t + 0x184 + i * 4);
        if (target != 0) {
            if (target == kHashZone) {
                fn_8012EB74(gWorldState, event, 0, 0, 0, 0);
            } else {
                EventSendTo(gEventMgr, target, t, event, 0, 0);
            }
        }
    }
}

// 0x800E9494
// Starts the monster's attack animation: every limb except slot 1 gets its reaction restarted with
// mode 3 (2 when the fighter's slot 3 health is 0 or less), then the character reaction starts.
void PlayMonsterAttackAnimation(u32 /*0x55*/ a0) {
    u8* fighter = (u8*)a0;
    if (fighter == NULL) return;
    s32 mode = 3;
    if (fn_8004E2A4(fighter, 3) <= 0) mode = 2;
    for (s32 i = 0; i < 5; i++) {
        u8* limb = *(u8**)(fighter + 0xC50 + i * 4);
        if (limb != NULL && i != 1) fn_8004F310(limb, 1, mode, 0);
    }
    CharacterStartReaction(fighter);
}

// 0x800F4C9C
// Plays a particle effect (a1) on one of the object's emitters (hash a2), unless the object is a
// battle fighter flagged 0x1000 whose slot is reported busy by fn_80212E28.
void PlayAttachedParticleEffectAtEmitter(void* a0, u32 a1, u32 a2) {
    u8* obj = (u8*)a0;
    if (obj == NULL) return;
    if ((*(u32*)(*(u8**)(obj + 4) + 0x1C) & 0x1000) &&
        (*(GetCCFn*)(*(u8**)obj + 0xCC))(obj) != NULL && gBattle != NULL &&
        fn_80212E28((u8**)((*(GetCCFn*)(*(u8**)obj + 0xCC))(obj) + 0x28))) {
        return;
    }
    u8* emitters = obj + 0x94;
    s32 idx = (s32)fn_801EC880(emitters, a2);
    if (idx >= 0) fn_801EC8C4(emitters, (u32)idx, a1);
}

// 0x800F5294
// Footstep sound for a character: finds the sound for the standing part's material (and the world's
// ground type), then plays it, positioned on the object except during phase 6.
void PlayFootStepSound(void* a0) {
    u8* obj = (u8*)a0;
    u8* state = (*(GetCCFn*)(*(u8**)obj + 0xCC))(obj);
    if (*(u8**)(state + 0x24) == NULL) return;

    state = (*(GetCCFn*)(*(u8**)obj + 0xCC))(obj);
    u8* slot = *(u8**)(state + 0x24);
    PartDef def;
    PartDbGetDef(&def, gGameDb, ***(s32***)(slot + 4));
    if (def.unk34 == 0xD) return;
    u32 material = (u32)def.unk3C;

    s32 ground = fn_80216754(gGameDb, *(u32*)(*(u8**)gWorldState + 0x1E4));
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        state = (*(GetCCFn*)(*(u8**)obj + 0xCC))(obj);
        u8* ch = *(u8**)(*(u8**)(state + 0x24) + 0x1C);
        if (ch != NULL) ch -= 0xC48;
        u8* link = *(u8**)(ch + 0xA00);
        if (link != NULL) {
            s32 g = fn_800D5800(link);
            if (g != -1) ground = g;
        }
    }
    if (*(s32*)((u8*)gGameDb + 0x10) != 1) return;
    if (ground == -1) return;

    u32 sound = (u32)fn_80216700(gGameDb, (s32)material, ground);
    if (WorldGetPhase(gGame + 0xD5F0) == 6) {
        EnsureSoundContext();
        PlaySound(sound, NULL, 0, 0, 0, 0);
        return;
    }
    EnsureSoundContext();
    PlaySound(sound, obj, 0, 0, 0, 0);
}

// 0x801254F8
// Plays the effect with the given key from the effect table (0xEC bytes per record, sorted when the
// table's flag at +0x14 is set, else searched linearly). Remembers it for the debris code, then spawns
// it at the prop's collider shapes ("Base" group) or, without collider, at its position.
void PropPlayFx(void* prop, u32 key) {
    u8* p = (u8*)prop;
    lbl_805FB2D8 = 0;
    gDebrisVolumeUnit = 0.0f;
    gDebrisVolume = 0.0f;

    u8* table = *(u8**)(lbl_8047C760 + 0x1C);
    s32 idx = -1;
    if (*(u8*)(table + 0x14) != 0) {
        s32 hi = *(s32*)(table + 0xC);
        s32 lo = -1;
        while (hi - lo > 1) {
            s32 sum = hi + lo;
            s32 mid = (sum + (s32)((u32)sum >> 31)) >> 1;
            u32 k = *(u32*)(*(u8**)(table + 0x10) + mid * 0xEC);
            if (key <= k) hi = mid;
            if (key > k) lo = mid;
        }
        if (key == *(u32*)(*(u8**)(table + 0x10) + hi * 0xEC)) idx = hi;
    } else {
        s32 count = *(s32*)(table + 0xC);
        for (s32 i = 0; i < count; i++) {
            if (key == *(u32*)(*(u8**)(table + 0x10) + i * 0xEC)) {
                idx = i;
                break;
            }
        }
    }
    u8* effect = NULL;
    if (idx >= 0) effect = *(u8**)(table + 0x10) + idx * 0xEC;
    gDebrisEffect = effect;
    if (effect == NULL) return;

    u8* collider = *(u8**)(p + 0x8C);
    if (collider != NULL) {
        if (*(s32*)(effect + 8) == 1) {
            f32 pos[4] = { *(f32*)(p + 0x18), *(f32*)(p + 0x1C), *(f32*)(p + 0x20), 0.0f };
            SpawnEffectAtPos(lbl_8047C760, effect, pos, NULL);
            return;
        }
        fn_801B154C(collider, kHashBase, (ShapeFn)fn_80124824, NULL);
        fn_801B154C(*(u8**)(p + 0x8C), kHashBase, (ShapeFn)DebrisSpawnForCollider, NULL);
    } else {
        u8* at = *(u8**)(p + 4);
        f32 pos[4] = { *(f32*)(at + 0), *(f32*)(at + 4), *(f32*)(at + 8), 0.0f };
        SpawnEffectAtPos(lbl_8047C760, effect, pos, NULL);
    }
}

// 0x8012341C
// Number of save blocks that still have to be freed: the blocks needed (5, or 4 when the save
// directory exists but the save file does not) minus the free ones, in units of 8 (rounded up);
// 0 when enough are free.
ScriptValue SaveManager_GetNumBlocksNeededToBeFreed(void) {
    u32 freeBlocks, freeInodes;
    fn_8026829C(&freeBlocks, &freeInodes);
    u32 needed = 5;
    if (SaveFileExists(lbl_805F8FD4, lbl_805F8FD4) && !SaveFileExists(lbl_805F8FD4, lbl_805F8FD0)) {
        needed = 4;
    }
    if (freeBlocks >= needed) return 0;
    return (ScriptValue)((needed - freeBlocks + 7) >> 3);
}

// 0x8012349C
// 1 when the save directory and the save file both exist, or when the NAND check (blocks 5/inodes 2
// without a save directory, 4/1 with one) reports nothing to free. Otherwise sets the save error flag
// and returns 0.
u32 /*0x06*/ SaveManager_HasEnoughFreeMemory(void) {
    if (SaveFileExists(lbl_805F8FD4, lbl_805F8FD4) && SaveFileExists(lbl_805F8FD4, lbl_805F8FD0)) {
        return 1;
    }
    u32 answer;
    if (!SaveFileExists(lbl_805F8FD4, lbl_805F8FD4)) {
        NANDCheck(5, 2, &answer);
    } else {
        NANDCheck(4, 1, &answer);
    }
    BOOL ok = (answer == 0);
    if (!ok) lbl_805FB2D5 = 1;
    return ok;
}

// 0x80142680
// Returns the seconds since the timer's stored time and stores the current time. When the stored
// time is slightly in the future (the clock went back by less than 60000000 ticks) it is first reset
// to now. The timebase is the bus clock / 4.
f32 TimerReset(u8* timer) {
    OSTime now = OSGetTime();
    u64 last = ((u64)*(u32*)(timer + 0) << 32) | *(u32*)(timer + 4);
    if ((u64)last - (u64)now < 60000000ull) {
        *(u32*)(timer + 4) = (u32)now;
        *(u32*)(timer + 0) = (u32)((u64)now >> 32);
    }
    u64 stored = ((u64)*(u32*)(timer + 0) << 32) | *(u32*)(timer + 4);
    f64 ticks = (f64)(s64)((u64)now - stored);   // fn_8032F9E8: s64 to f64
    *(u32*)(timer + 4) = (u32)now;
    *(u32*)(timer + 0) = (u32)((u64)now >> 32);
    f64 timebase = (f64)(OSGetBusClock() >> 2);
    return (f32)(ticks / timebase);
}

// 0x80147704
// Reads a stick from its register entry (top 3 bits must be 3): bits 0-15 are the angle (signed, 1/4096
// of a half turn per step), bits 16-28 the magnitude (0..8191, scaled to 0..1). Writes x and y, or
// zeros when the entry is not a stick.
void StickRead(s32 arg0, f32* x, f32* y) {
    u32 v = (u32)fn_801478C8(lbl_805FB490, arg0);
    if ((v >> 29) != 3) {
        *x = 0.0f;
        *y = 0.0f;
        return;
    }
    f32 angle = 0.00024414062f * (f32)(s16)(v & 0xFFFF);
    f32 mag = 0.00012208521f * (f32)((v >> 16) & 0x1FFF);
    *x = mag * (f32)cosf(1.5707964f * angle);
    *y = mag * (f32)sinf(1.5707964f * angle);
}

// 0x8014855C
// Resets the GX render state to the engine defaults: colours, lights, projection, per-channel
// state and the fixed pipeline switches.
void RendSetDefaultState(void) {
    f32 clear[4], white[4];
    f32 c0[4], c1[4], c2[4], c3[4], c4[4], c5[4];
    Vec3 zero, unit, a, b, c;
    f32 identity[4];

    fn_80148770(3);
    fn_8014879C(clear);
    fn_801487B4(white);
    f32* w0 = fn_80148D70(c0, white);
    f32* x0 = fn_80148D70(c1, clear);
    f32* x1 = fn_80148D70(c2, clear);
    f32* x2 = fn_80148D70(c3, clear);
    fn_801487CC(x2, x1, x0, w0);
    fn_80148D94(fn_80148D70(c4, white));
    for (s32 i = 0; i < 8; i++) fn_80156894(i);
    fn_80155D70();
    fn_80154974();
    fn_801566E8(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
    Vec3SetZero(&zero);
    fn_80149094(&unit);
    for (s32 i = 0; i < 4; i++) {
        Vec3Assign(&a, &zero);
        Vec3Assign(&b, &zero);
        fn_801490AC(i, &b, &a, 0.0f, 0.0f);
        fn_8014930C(i, fn_80148D70(c5, clear));
        Vec3Assign(&c, &unit);
        fn_80149428(i, &c);
        fn_80149464(i, 0, 0);
        fn_80149468(i);
    }
    fn_801494B4();
    fn_8014950C(1, 1);
    fn_8010AB0C(1, 1);
    fn_8014879C(identity);
    fn_8015150C(identity, 0.0f, 1.0f);
    fn_80149540();
    fn_80154294();
    fn_80044C0C();
    fn_8010AB70();
    fn_801514F0();
    fn_80149594();
    fn_8014970C(6, 8);
    fn_8010AAE0();
    fn_80154318();
    fn_80149744(2);
    fn_8015428C(1);
    fn_80154290(0);
    fn_8014977C(0.0f);
    fn_80149780();
    fn_801497AC();
    fn_801497B0(1.0f);
}

// 0x8018DCA4
// Removes the UI cursor: when a cursor screen is set (+0x7C) its three widgets are reset (two get a
// zero tint, the third is hidden), the cursor key (+0x78) is removed from the list and the cursor
// screen is hidden and cleared. Logs when the cursor screen is gone.
void ScreenSetsFlush(void* sets) {
    u8* s = (u8*)sets;
    if (*(u8**)(s + 0x7C) == NULL) return;

    u8* w = fn_8019BF64(*(u8**)(s + 0x7C), 0x0517EA02);
    if (w != NULL) {
        fn_8019BC80(w, 0);
        *(f32*)(w + 0x80) = 0.0f;
        *(f32*)(w + 0x84) = 0.0f;
        *(f32*)(w + 0x88) = 0.0f;
        *(f32*)(w + 0x8C) = 0.0f;
        (*(UIColorFn*)(*(u8**)(w + 0x138) + 0x2C))(w, 0x20, 0.0f, 0.0f, 0.0f, 0.003921569f);
    }
    w = fn_8019BF64(*(u8**)(s + 0x7C), 0x9C1EBBB8);
    if (w != NULL) {
        fn_8019BC80(w, 0);
        *(f32*)(w + 0x80) = 0.0f;
        *(f32*)(w + 0x84) = 0.0f;
        *(f32*)(w + 0x88) = 0.0f;
        *(f32*)(w + 0x8C) = 0.0f;
        (*(UIColorFn*)(*(u8**)(w + 0x138) + 0x2C))(w, 0x20, 0.0f, 0.0f, 0.0f, 0.003921569f);
    }
    w = fn_8019BF64(*(u8**)(s + 0x7C), 0x382C6918);
    if (w != NULL) (*(UIVisFn*)(*(u8**)(w + 0x138) + 0x30))(w, 2);

    if (fn_801B3308() != NULL) fn_801B3378(fn_801B3308(), *(u32*)(s + 0x78));
    u8* cursor = *(u8**)(s + 0x7C);
    if (cursor != NULL) {
        (*(UIVisFn*)(*(u8**)(cursor + 0x138) + 0x30))(cursor, 2);
        if (fn_801B3308() != NULL) fn_801B3378(fn_801B3308(), *(u32*)(s + 0x78));
    } else {
        fn_8018D2B8(lbl_80381318);   // "UIEngine::ShowCursor : cursor not defined ..."
    }
    *(u8**)(s + 0x7C) = NULL;
}

// 0x8018F2BC
// Script callback: enables or disables the activation grid of the current screen's active widget.
// Logs when there is no active screen set, no screen widgets or the widget has no grid (+0x144).
void UIActivationGrid_SetEnabled(u32 /*0x06*/ a0) {
    if (ScreenSetsCurrent(gScreenSets) == NULL) {
        fn_8018D370(lbl_80381868 + 0x40);
        return;
    }
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    u8* el;
    if (set == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x40);
        el = NULL;
    } else {
        s32 count = *(s32*)(set + 0x60);
        if (count <= 0) {
            fn_8018D2B8(lbl_80381868 + 0x14C);
            el = NULL;
        } else {
            el = *(u8**)(set + count * 4 + 0x3C);
            if (el == NULL) {
                fn_8018D370(lbl_80381868 + 0x170);
                el = NULL;
            }
        }
    }
    if (el != NULL) {
        if (*(u8**)(el + 0x144) == NULL) {
            fn_8018D2B8(lbl_80381868 + 0x190);
            return;
        }
        fn_8019AA10(*(u8**)(el + 0x144), a0);
    }
}

// 0x8018F580
// Script callback: calls the reset slot (vtable +0xC) of an element of the current screen set.
void UIElement_Reset(u32 a0) {
    void* set = ScreenSetsCurrent(gScreenSets);
    void* el;
    if (set == NULL) {
        fn_8018D370(lbl_803818A8);
        el = NULL;
    } else {
        el = ScreenSetFindElement(set, a0);
        if (el == NULL) {
            fn_8018D2B8(lbl_80381AA0, fn_801EB708(a0));
            el = NULL;
        }
    }
    if (el != NULL) (*(UIVFn*)(*(u8**)((u8*)el + 0x138) + 0xC))(el);
}

// 0x801C81D8
// Spawns an effect (a record with a count at +4 and 0x1C-byte emitter entries from +0xC) in the effect
// manager. Fails (0) when the pool limit would be exceeded. Each emitter entry gets its effect object
// (initialised by fn_801C7B20 at pos); infinite-life emitters without an owner are refused with a log
// message. owner, when given, receives the new effect record. Returns 1 on success.
BOOL SpawnEffectAtPos(u8* mgr, u8* effect, const f32* pos, u8** owner) {
    if (effect == NULL) return 0;
    if (*(s32*)(mgr + 4) + *(s32*)(effect + 4) > **(s32**)(mgr + 0x18)) return 0;

    *(s32*)(mgr + 0) += 1;
    u8* node = (u8*)fn_801EB94C(*(void**)(mgr + 0xC));
    if (node == NULL) {
        *(s32*)(mgr + 0) -= 1;
        return 0;
    }
    *(u8**)(node + 4) = effect;
    s32 count = *(s32*)(effect + 4);
    *(s32*)(node + 0xC) = count;
    *(s32*)(node + 0x10) = count;
    *(u8*)(node + 8) = 0;
    *(u8***)(node + 0) = owner;

    for (s32 i = 0; i < *(s32*)(effect + 4); i++) {
        u8* desc = *(u8**)(effect + i * 0x1C + 0xC);
        f32 life = *(f32*)(desc + 0x184);
        if (!(life >= 0.0f) && owner == NULL) {
            char buf[0x230];
            SysSprintf(buf, "EmitterDesc error: %s : attempting to add infinite life emitter without an owner.",
                       fn_801EB708(*(u32*)(desc + 4)));
            *(u8**)(node + 0x14 + i * 4) = NULL;
        } else {
            u8* fx = NULL;
            u8* pool = *(u8**)(mgr + 0x18);
            if (*(s32*)(mgr + 4) < *(s32*)pool) {
                fx = (u8*)fn_801EB94C(pool);
                if (fx != NULL) {
                    fn_801C7B20(fx, desc, pos, mgr);
                    *(u32*)fx |= 2;
                }
            }
            *(u8**)(node + 0x14 + i * 4) = fx;
            if (fx != NULL) {
                *(u8**)(fx + 0x68) = node;
                *(u8**)(*(u8**)(node + 0x14 + i * 4) + 0x6C) = node + 0x14 + i * 4;
                *(u8**)(*(u8**)(node + 0x14 + i * 4) + 0x70) = effect + 0xC + i * 0x1C;
            }
        }
    }
    if (owner != NULL) *owner = node;
    return 1;
}

// 0x801D8E80
// Loads a texture by name: builds "textures/<name>", takes the file part after the last slash and
// looks it up in the texture cache (bumping the reference count). Otherwise it loads the file
// (retrying once) and registers it. Then the same with "_0" appended. Returns the texture or NULL.
void* TextureLoad(const char* name) {
    char path[0x100];
    strcpy(path, "textures/");   // 0x80384420
    strcat(path, name);
    s32 n = (s32)strlen(path);
    const char* file = path;
    for (s32 i = n; i >= 0; i--) {
        char c = path[i];
        if (c == '\\' || c == '/') {
            file = path + i + 1;
            break;
        }
    }

    u8* tex = fn_8016A08C(file);
    if (tex != NULL) {
        *(s32*)(tex + 0x24) += 1;
        return tex;
    }
    tex = fn_801D8D80(path, file);
    if (tex != NULL) {
        fn_8016964C(lbl_805FB5A8, tex);
        return tex;
    }
    tex = fn_801D8D80(path, file);
    if (tex != NULL) {
        fn_8016964C(lbl_805FB5A8, tex);
        return tex;
    }

    strcat(path, "_0");   // 0x805F961C
    tex = fn_8016A08C(file);
    if (tex != NULL) {
        *(s32*)(tex + 0x24) += 1;
        return tex;
    }
    tex = fn_801D8D80(path, file);
    if (tex != NULL) {
        fn_8016964C(lbl_805FB5A8, tex);
        return tex;
    }
    tex = fn_801D8D80(path, file);
    if (tex != NULL) {
        fn_8016964C(lbl_805FB5A8, tex);
        return tex;
    }
    return NULL;
}

// 0x8020AF2C
// Stops the music: when the fade timer (lbl_805F9970) is non-negative, applies the clamped target
// level (0..1) immediately and cancels the fade; then stops the audio manager's music.
void StopMusic(void) {
    if (lbl_805F9970 >= 0.0f) {
        f32 v = lbl_805F997C;
        f32 c = 0.0f;
        if (v >= 0.0f) {
            c = 1.0f;
            if (v <= 1.0f) c = v;
        }
        lbl_805F9978 = c;
        fn_801FFC04(gAudio, c);
        lbl_805F9974 = -1.0f;
        lbl_805F9970 = -1.0f;
    }
    fn_801FD730(gAudio);
    lbl_805FBBB3 = 0;
}

}  // extern "C"
