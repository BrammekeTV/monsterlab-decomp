// Misc functions from the m2c draft batch "miss50": UI sound list callbacks, particle unspawn,
// world zone bookkeeping, and assorted 0x8000xxxx-0x8004xxxx routines.
#include "types.h"
#include "game/part.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void* ScreenSetsCurrent(void* sets);                    // 0x8018D1E4
void  fn_8018D2B8(const char* fmt, ...);                // 0x8018D2B8 (debug log)
void  fn_8018D370(const char* fmt, ...);                // 0x8018D370 (debug log)
const char* FormatHashString(u32 hash);                      // 0x801EB708 (hash -> name for logs)
void* UIScreenSet_FindSoundList(void* set, u32 hash);                 // 0x80198FCC
s32   UISoundList_FindSound(void* obj, u32 hash);                 // 0x8019ADFC
u32   UISoundList_GetSoundByIndex(void* obj, s32 index);                // 0x8019AD84
void  UIScreen_PlayLoopingSoundFX(void* el, void* obj);                 // 0x80199B68
s32   PlaySoundAt(void* sound, int a, const void* pos, int b);  // 0x80209ABC
void* fn_801D40E0(s32 n);                               // 0x801D40E0
void* fn_80209814(void* v);                             // 0x80209814
void* EventPost(s32 id, void* sender);                  // 0x801F3918
u32   fn_801F38F8(u8* p);                               // 0x801F38F8
void  fn_801C83AC(u8* mgr, u8* item);                   // 0x801C83AC
void  fn_801C849C(u8* mgr, u8* item);                   // 0x801C849C

u8*   GetGlobalSettings(void);
void  GameLog(int level, int category, const char* fmt, ...);
void  EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);
BOOL  ZoneIsLoaded(void* world, s32 zone);
void  ZoneSlotInit(void* slot);
s32   fn_801D3A60(s32 zone);                            // 0x801D3A60
void  HudShowConversation(void* hud, s32 conversation); // 0x8001B0B8
void  HudUpdateScreens(void* screens, f32 dt);          // 0x801BB5BC
void  fn_8001B164(u8* hud);                             // 0x8001B164
void  fn_8001B128(void* hud);                           // 0x8001B128
u8*   fn_8001B044(u8* hud, u32 key);                    // 0x8001B044
void  fn_801BBB68(void* mgr, s32 id, int a, int b);     // 0x801BBB68
void fn_80051940(f32 dt);                              // 0x80051940
void  SessionUpdateDisabled(u8* p, f32 dt);             // 0x80012B5C
void  fn_80012CB0(u8* p, f32 dt);                       // 0x80012CB0

void  MonsterBuildPreset(u8* m, s32 preset);
void  MonsterSetName(void* monster, const char* name);
void* fn_8004DAF8(void* c, s32 a, s32 b);               // 0x8004DAF8
void  PartAnimKey(char* buf, s32 a, s32 b);             // 0x802152FC
char* strcat(char* dst, const char* src);               // 0x80338898
u32   StringHash(const char* s);                        // 0x801EB704
int   SysSprintf(char* buf, const char* fmt, ...);      // 0x80142EFC
void* CharacterFind(s32 id);                            // 0x800E876C
void* fn_8003DB78(u32 hash, s32 a, s32 b);              // 0x8003DB78
char* fn_80338BC0(char* s, const char* sub);            // 0x80338BC0 (MSL strstr)
void* fn_801F06FC(void);                                // 0x801F06FC
void  fn_801DB398(u8* a, u32 hash, u8* b);     // 0x801DB398
void  fn_8020E864(u8* p, s32 del);                      // 0x8020E864
void* ScreenSetsFind(void* sets, s32 zone);

extern f32 lbl_805FD25C;            // 0x805FD25C
extern f32 lbl_805FD268;            // 0x805FD268
extern const char lbl_805F8200[];   // 0x805F8200 (empty string)
extern const char lbl_80354800[];   // 0x80354800 (string)
extern const char lbl_8035480C[];   // 0x8035480C (string)
extern s32 lbl_805FB034;            // 0x805FB034
extern s32 lbl_803C4E28[];          // 0x803C4E28 (combat part zone-loaded flags)
extern u8 lbl_805FB030[];           // 0x805FB030
extern void* lbl_803C4F60[];        // 0x803C4F60
void* ScreenSetFindElement(void* set, u32 hash);        // 0x80198F18
void  fn_800311E8(u8* box, f32* pt);                    // 0x800311E8
void  fn_800E6360(f32 dt);                              // 0x800E6360
void  fn_8003CF7C(f32 dt);                              // 0x8003CF7C
void  fn_8012570C(f32 dt);                              // 0x8012570C
void  fn_800D3818(u8* objDb, f32 dt);                              // 0x800D3818
void  fn_800D6E14(f32 dt);                              // 0x800D6E14
void fn_8010A678(void);                              // 0x8010A678
void  fn_8010A76C(u8* p, f32 dt);                       // 0x8010A76C
void  fn_801214D8(u8* p, f32 a, f32 dt);                // 0x801214D8
void  fn_801CE7FC(u8* p, f32 dt);                       // 0x801CE7FC
void  fn_801BE44C(u8* p, f32 dt);                       // 0x801BE44C
void  fn_801C3C2C(u8* p, f32 dt);                       // 0x801C3C2C
void  fn_801B420C(void* p);                             // 0x801B420C
void  fn_801D8BB4(u8* p, f32 dt);                       // 0x801D8BB4
void  fn_801E28DC(u8* p, f32 dt);                       // 0x801E28DC
u32   fn_801F2C10(u32 key);                             // 0x801F2C10
BOOL  fn_801F5220(void);                                // 0x801F5220
void fn_801DFB58(u8* cmd, s32 mode);                        // 0x801DFB58
void  WorldTick(u8* world, f32 dt);                     // 0x8012AB00
void  ZoneEffectsUpdate(void* effects, f32 dt);         // 0x801E97FC
u8*   fn_8015EC9C(u8* tbl, f32 a, f32 b);               // 0x8015EC9C
void  fn_8020B024(f32 v);                               // 0x8020B024
void  fn_8020B078(f32 v);                               // 0x8020B078
void  fn_8020B160(f32 v);                               // 0x8020B160
u32   fn_8020A9A8(s32 handle);                          // 0x8020A9A8
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);  // 0x80209E70
u32   RandomU32(void);                                  // 0x80143460
BOOL  MonsterHasPart(u8* m, const PartInstance* p);
u8    fn_8021EE94(u8* p);                               // 0x8021EE94
void  fn_800126A0(u8* a, void* world, u8* b, s32 n);    // 0x800126A0
void  fn_800D46B0(void* objDb);                         // 0x800D46B0
BOOL  RegisterMovingCharacter(u8* chr);                 // 0x80012F78
void* memset(void* p, int c, size_t n);
void* memcpy(void* d, const void* s, size_t n);
s32   RegisterScriptFunc(const char* name, s32 type, void* fn);  // 0x801DC91C
BOOL  IsOnlineBattle(void);
void  ZoneLog(void* loader, const char* fmt, ...);
s32   PartGetCurrentHealth(const PartInstance* p);
s32   PartGetMaxHealth(PartInstance* p);
s8    PartGetQuality(const PartInstance* p);
void  PartSetHealth(PartInstance* p, s32 hp);
u8*   MonsterCtor(u8* m);
u8*   MonsterDtor(u8* m, s32 del);
u8*   SaveGetCurrentMonster(u8* s);
void  fn_8003E000(u8* a, u8* b, u8* c);                 // 0x8003E000
void  fn_8003D964(u8* a);                               // 0x8003D964


extern u8 lbl_8047C790[];           // 0x8047C790
extern u8 gIdPool[];                // 0x803xxxxx (zone loader / id pool)
extern u8 lbl_8047C7AC[];           // 0x8047C7AC
extern u8 lbl_8047C7D8[];           // 0x8047C7D8
extern u8 lbl_8047C640[];           // 0x8047C640
extern u8 lbl_803C2A20[];           // 0x803C2A20
extern u8 lbl_8040D500[];           // 0x8040D500
extern u8 lbl_805FB8A8[];           // 0x805FB8A8
extern u8 lbl_80355940[];           // 0x80355940 (4 sound handles)
extern u8* lbl_805FB134;            // 0x805FB134 (experiment object)
extern u8* lbl_805FAF78;            // 0x805FAF78
void* fn_80020590(void* a0);                            // 0x80020590
void* fn_80020480(void* a0);                            // 0x80020480
void* fn_80020758(void* a0);                            // 0x80020758
void* fn_80020864(void* a0);                            // 0x80020864
void* fn_8002104C(void* a0);                            // 0x8002104C
void* fn_8002112C(void* a0);                            // 0x8002112C
void* fn_80021240(void* a0);                            // 0x80021240
void* fn_80020A04(void* a0);                            // 0x80020A04
void* fn_80020D28(void* a0);                            // 0x80020D28
void* fn_80021354(void* a0);                            // 0x80021354
extern u8* lbl_805FB188;           // 0x805FB188 (spawn record table)
extern u8 lbl_805FB130;            // 0x805FB130
extern void* lbl_805FB964;          // 0x805FB964 (movie player)
extern s32 lbl_805FB970;            // 0x805FB970
extern s32 lbl_805FB974;            // 0x805FB974
extern s32 lbl_805FB9D0;            // 0x805FB9D0
extern u32 lbl_805FAE68;            // 0x805FAE68
extern f32 lbl_805FAE64;            // 0x805FAE64
extern f32 lbl_805FB314;            // 0x805FB314
extern s32 lbl_805FB01C;            // 0x805FB01C
extern s32 lbl_805FB020;            // 0x805FB020
extern s32 lbl_805FB038;            // 0x805FB038
extern s8 lbl_805FB025;             // 0x805FB025
extern const char lbl_80375BDC[];   // 0x80375BDC (ZoneLoadAsync: queue full message)
extern u8 gGame[];
extern u8 lbl_803C4298[];           // 0x803C4298
extern u8* lbl_805FBBE0;            // 0x805FBBE0 (save block pointer)
extern f32 lbl_805FB918;            // 0x805FB918
extern const char lbl_80381868[];   // 0x80381868 (UI string block)
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
extern u8 lbl_8047C760[];           // 0x8047C760 (effect manager)
}
extern "C++" void* gScreenSets;     // 0x805FB654
extern "C++" {
struct ScriptCtx;
void acquire_camera_semaphore(ScriptCtx* ctx);
void release_camera_semaphore(ScriptCtx* ctx);
void set_igc_mode(ScriptCtx* ctx);
void set_igc_mode_from_sensor_param(ScriptCtx* ctx);
void clear_igc_mode(ScriptCtx* ctx);
void SetCameraMode(ScriptCtx* ctx);
void set_camera_mode_from_sensor_type(ScriptCtx* ctx);
void clear_camera_mode(ScriptCtx* ctx);
void set_camera_location(ScriptCtx* ctx);
void end_trigger_camera(ScriptCtx* ctx);
void send_event(ScriptCtx* ctx);
void send_request(ScriptCtx* ctx);
void send_request_complete(ScriptCtx* ctx);
void ping(void);
void broadcast_on_done_event(ScriptCtx* ctx);
void set_camera_location_from_sensor_param(ScriptCtx* ctx);
void set_timeout_from_sensor_param(ScriptCtx* ctx);
BOOL should_igc_abort(ScriptCtx* ctx);
BOOL special_camera_done_slerping(ScriptCtx* ctx);
void set_interp_timer(ScriptCtx* ctx);
void fixed_camera_return(ScriptCtx* ctx);
void force_fixed_camera_return(ScriptCtx* ctx);
}
extern "C++" s32 gZoneResourceSize; // 0x805FB94C
struct StringTable;
extern "C++" StringTable* gStringTable;  // 0x805FB910
extern "C++" void* gStringTable2;        // 0x805FB90C
extern "C++" void* gEventMgr;      // 0x805FB6C8
extern "C++" void* gObjDb;          // 0x805FB180
extern "C++" u8 gWorldState[];      // 0x803E6D00

extern "C" {

// Shared body of the UI sound list callbacks: finds the sound list by hash in the current screen
// set, the entry by hash in the list, makes sure the voice SFX handle exists, then plays it.
static void UISoundList_Play(u32 listHash, u32 entryHash, int flag) {
    void* set = ScreenSetsCurrent(gScreenSets);
    void* list = UIScreenSet_FindSoundList(set, listHash);
    if (list == NULL) {
        fn_8018D370(lbl_80381868 + 0x674, listHash);
        return;
    }
    s32 idx = UISoundList_FindSound(list, entryHash);
    if (idx == -1) {
        fn_8018D370(lbl_80381868 + 0x6d8, FormatHashString(entryHash));
        return;
    }
    u32 sound = UISoundList_GetSoundByIndex(list, idx);
    if (sound == 0) {
        fn_8018D370(lbl_80381868 + 0x708, idx);
        return;
    }
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySoundAt((void*)(intptr_t)sound, 0, NULL, flag);
}

// 0x80194AF4
// Plays a sound from a UI sound list (flag 0).
void UISoundList_PlaySfx(u32 a0, u32 a1) {
    UISoundList_Play(a0, a1, 0);
}

// 0x80194D04
// Plays a voice sound from a UI sound list (flag 2).
void UISoundList_PlayVoiceSfx(u32 a0, u32 a1) {
    UISoundList_Play(a0, a1, 2);
}

// 0x80194F14
// Looks up the sound list entry, then hands it to UIScreen_PlayLoopingSoundFX on the top element of the
// current screen set (logging when there is no set, no element, or the top slot is empty).
void UITopScreen_PlayLoopingSfx(u32 a0, u32 a1) {
    void* set = ScreenSetsCurrent(gScreenSets);
    void* list = UIScreenSet_FindSoundList(set, a0);
    if (list == NULL) {
        fn_8018D370(lbl_80381868 + 0x674, a0);
        return;
    }
    s32 idx = UISoundList_FindSound(list, a1);
    if (idx == -1) {
        fn_8018D370(lbl_80381868 + 0x6d8, FormatHashString(a1));
        return;
    }
    u32 sound = UISoundList_GetSoundByIndex(list, idx);
    if (sound == 0) {
        fn_8018D370(lbl_80381868 + 0x708, idx);
        return;
    }
    u8* cur = (u8*)ScreenSetsCurrent(gScreenSets);
    u8* top;
    if (cur == NULL) {
        fn_8018D2B8(lbl_80381868 + 0x40);
        top = NULL;
    } else {
        s32 n = *(s32*)(cur + 0x60);
        if (n <= 0) {
            fn_8018D2B8(lbl_80381868 + 0x14c);
            top = NULL;
        } else {
            top = *(u8**)(cur + 0x3c + n * 4);
            if (top == NULL) fn_8018D370(lbl_80381868 + 0x170);
        }
    }
    if (top != NULL) UIScreen_PlayLoopingSoundFX(top, (void*)(intptr_t)sound);
}

// 0x801DC088
// Detaches the particle effect stored at +0xA8: posts a stop event (0x6A when fn_801F38F8 says
// so, else 0x69 if the id at +0x30 is valid) and releases the effect via the manager.
void UnspawnAttachedParticle(void* a0, u32 a1) {
    u8* p = (u8*)a0;
    if (p != NULL && *(s32*)(p + 0xA8) != 0) {
        if (fn_801F38F8(p) != 0) {
            u8* ev = (u8*)EventPost(0x6A, p);
            if (ev != NULL) {
                *(s32*)(ev + 0x4) = 0;
                *(s32*)(ev + 0x8) = (a1 != 0) ? 2 : 3;
            }
        } else if (*(s32*)(p + 0x30) != -1) {
            u8* ev = (u8*)EventPost(0x69, NULL);
            if (ev != NULL) {
                *(s32*)(ev + 0x8) = 0;
                *(s32*)(ev + 0xC) = (a1 != 0) ? 2 : 3;
                *(s32*)(ev + 0x4) = *(s32*)(p + 0x30);
            }
        }
        if (a1 != 0) {
            fn_801C83AC(lbl_8047C760, *(u8**)(p + 0xA8));
        } else {
            fn_801C849C(lbl_8047C760, *(u8**)(p + 0xA8));
        }
        *(s32*)(p + 0xA8) = 0;
    }
}

// 0x80132344
// Removes the zone entry (16-byte records at world+0x284, count at +0x290) matching zone:
// calls its callback (+4) with the zone resource and the record's arg (+8), then shifts the
// following records down.
void WorldSetZoneActive(u8* world, s32 zone) {
    s32 n = *(s32*)(world + 0x290);
    s32 i = 0;
    while (i < n && zone != *(s32*)(*(u8**)(world + 0x284) + i * 0x10)) i++;
    if (i < n) {
        u8* rec = *(u8**)(world + 0x284) + i * 0x10;
        typedef void (*Cb)(u8*, s32);
        Cb cb = *(Cb*)(rec + 0x4);
        if (cb != NULL) {
            cb(*(u8**)(*(u8**)world + 0x1A4) + zone * gZoneResourceSize, *(s32*)(rec + 0x8));
        }
        while (i < *(s32*)(world + 0x290) - 1) {
            u8* t = *(u8**)(world + 0x284) + i * 0x10;
            i++;
            *(s32*)(t + 0x0) = *(s32*)(t + 0x10);
            *(s32*)(t + 0x4) = *(s32*)(t + 0x14);
            *(s32*)(t + 0x8) = *(s32*)(t + 0x18);
            *(s32*)(t + 0xC) = *(s32*)(t + 0x1C);
        }
        *(s32*)(world + 0x290) = *(s32*)(world + 0x290) - 1;
    }
}

// Sum of the 13 capacity counters in the global settings (+0x34..+0x64): total zone slots.
static s32 ZoneSlotCapacity(void) {
    s32 n = 0;
    n += *(s32*)(GetGlobalSettings() + 0x4c);
    n += *(s32*)(GetGlobalSettings() + 0x34);
    n += *(s32*)(GetGlobalSettings() + 0x50);
    n += *(s32*)(GetGlobalSettings() + 0x54);
    n += *(s32*)(GetGlobalSettings() + 0x58);
    n += *(s32*)(GetGlobalSettings() + 0x5c);
    n += *(s32*)(GetGlobalSettings() + 0x60);
    n += *(s32*)(GetGlobalSettings() + 0x64);
    n += *(s32*)(GetGlobalSettings() + 0x38);
    n += *(s32*)(GetGlobalSettings() + 0x3c);
    n += *(s32*)(GetGlobalSettings() + 0x40);
    n += *(s32*)(GetGlobalSettings() + 0x44);
    n += *(s32*)(GetGlobalSettings() + 0x48);
    return n;
}

// Entity of a zone resource by flat index: first the static list (stride-0x40 records at +0x8C,
// pointers at +0x120, count at +0x94), then the dynamic list (+0x90 / +0x12C).
static void ZoneEntity(u8* zr, s32 i, u8** rec, u8** item) {
    s32 n = *(s32*)(zr + 0x94);
    if (i < n) {
        *item = *(u8**)(*(u8**)(zr + 0x120) + i * 4);
        *rec = *(u8**)(zr + 0x8c) + i * 0x40;
    } else {
        *item = *(u8**)(*(u8**)(zr + 0x12c) + (i - n) * 4);
        *rec = *(u8**)(zr + 0x90) + (i - n) * 0x40;
    }
}

// 0x8012E93C
// Sends event to every entity of zone 0's resource whose record passes the filters (bit idx of
// +0x1C if idx != 0, mask against +0x20 if mask != 0) and whose owner at +4 has the u16 at +0x30
// equal to zone; then the same for the resource of zone itself when it is loaded (no owner check).
void WorldBroadcastToZone(void* unused, u32 event, s32 a, s32 b, s32 idx, s32 zone, s32 mask) {
    s32 bit = 1 << idx;
    u8* zr0 = *(u8**)(*(u8**)gWorldState + 0x1a4);
    s32 total = *(s32*)(zr0 + 0x94) + *(s32*)(zr0 + 0x98);
    for (s32 i = 0; i < total; i++) {
        u8* rec;
        u8* item;
        ZoneEntity(*(u8**)(*(u8**)gWorldState + 0x1a4), i, &rec, &item);
        if ((idx == 0 || (*(s32*)(rec + 0x1c) & bit)) && (mask == 0 || (*(s32*)(rec + 0x20) & mask)) &&
            item != NULL) {
            u8* owner = *(u8**)(item + 0x4);
            if (owner != NULL && (s32)*(u16*)(owner + 0x30) == zone) {
                EventSend(gEventMgr, item, item, event, a, b, 1, 0);
            }
        }
    }
    if (ZoneIsLoaded(gWorldState, zone) && zone >= 0) {
        u8* zr = *(u8**)(*(u8**)gWorldState + 0x1a4) + zone * gZoneResourceSize;
        s32 total2 = *(s32*)(zr + 0x94) + *(s32*)(zr + 0x98);
        for (s32 i = 0; i < total2; i++) {
            u8* rec;
            u8* item;
            ZoneEntity(*(u8**)(*(u8**)gWorldState + 0x1a4) + zone * gZoneResourceSize, i, &rec, &item);
            if ((idx == 0 || (*(s32*)(rec + 0x1c) & bit)) && (mask == 0 || (*(s32*)(rec + 0x20) & mask)) &&
                item != NULL) {
                EventSend(gEventMgr, item, item, event, a, b, 1, 0);
            }
        }
    }
}

// 0x8013022C
// Re-initialises the world's zone slots (0x30 bytes each, count = sum of the settings capacities)
// and resets the zone queue state.
void WorldResetQueues(u8* world) {
    s32 i = 0;
    s32 off = 0;
    while (i < ZoneSlotCapacity()) {
        ZoneSlotInit(*(u8**)(world + 0xb0) + off);
        i++;
        off += 0x30;
    }
    i = 0;
    while (i < ZoneSlotCapacity()) i++;  // original loops here with an empty body
    *(s32*)(world + 0x1a8) = -1;
    *(s32*)(world + 0xc0) = -1;
    *(s32*)(world + 0xe0) = 0;
    *(s32*)(*(u8**)world + 0xc) = 0;
    *(s32*)(world + 0xbc) = 0;
    *(u16*)(world + 0xc4) = 0;
    *(u8*)(world + 0xf4) = 0;
    *(u8*)(world + 0x110) = 1;
    *(u8**)(world + 0x1c0) = world + 0x1c4;
    *(s32*)(world + 0x1bc) = 0x10;
    *(s32*)(world + 0x1b8) = -1;
    *(s32*)(world + 0x1b4) = -1;
    *(s32*)(world + 0x1b0) = 0;
    *(s32*)(world + 0x290) = 0;
}

// 0x80132098
// Queues zone for loading: rejects invalid zone ids and a full queue; returns 1 if already
// queued; if the zone is already loaded clears bit 0x10 of its slot flags and runs cb(zoneRes,
// arg) right away; otherwise appends {zone, cb, arg, flags} to the 16-byte queue at +0x284.
s32 ZoneLoadAsync(u8* world, s32 zone, s32 flags, void (*cb)(u8*, s32), s32 arg) {
    if (zone < 1) return 0;
    s32 maxZone = *(s32*)(GetGlobalSettings() + 0x80);
    maxZone += *(s32*)(GetGlobalSettings() + 0x68);
    maxZone += *(s32*)(GetGlobalSettings() + 0x84);
    maxZone += *(s32*)(GetGlobalSettings() + 0x88);
    maxZone += *(s32*)(GetGlobalSettings() + 0x8c);
    maxZone += *(s32*)(GetGlobalSettings() + 0x90);
    maxZone += *(s32*)(GetGlobalSettings() + 0x94);
    maxZone += *(s32*)(GetGlobalSettings() + 0x98);
    maxZone += *(s32*)(GetGlobalSettings() + 0x6c);
    maxZone += *(s32*)(GetGlobalSettings() + 0x70);
    maxZone += *(s32*)(GetGlobalSettings() + 0x74);
    maxZone += *(s32*)(GetGlobalSettings() + 0x78);
    maxZone += *(s32*)(GetGlobalSettings() + 0x7c);
    if (zone >= maxZone + 1) return 0;
    s32 n = *(s32*)(world + 0x290);
    if (n == ZoneSlotCapacity()) {
        GameLog(3, 0, lbl_80375BDC);
        return 0;
    }
    for (s32 i = 0; i < n; i++) {
        if (zone == *(s32*)(*(u8**)(world + 0x284) + i * 0x10)) return 1;
    }
    if (ZoneIsLoaded(world, zone)) {
        u8* slot = *(u8**)(world + 0xb0) + fn_801D3A60(zone) * 0x30;
        *(u32*)(slot + 0xc) &= ~0x10u;
        if (cb != NULL) cb(*(u8**)(*(u8**)world + 0x1a4) + zone * gZoneResourceSize, arg);
        return 1;
    }
    u8* rec = *(u8**)(world + 0x284) + *(s32*)(world + 0x290) * 0x10;
    *(s32*)rec = zone;
    rec = *(u8**)(world + 0x284) + *(s32*)(world + 0x290) * 0x10;
    *(void**)(rec + 0x4) = (void*)cb;
    rec = *(u8**)(world + 0x284) + *(s32*)(world + 0x290) * 0x10;
    *(s32*)(rec + 0x8) = arg;
    rec = *(u8**)(world + 0x284) + *(s32*)(world + 0x290) * 0x10;
    *(s32*)(rec + 0xc) = flags;
    *(s32*)(world + 0x290) = *(s32*)(world + 0x290) + 1;
    return 1;
}

// 0x800D83C8
// Per-frame world/HUD update (only in state 5 of lbl_803C4298, and only when the object at
// gGame+0xD614, if used, allows it): shows queued conversations, updates the HUD screens, runs
// the session updates and counts down the timer at gGame+0x7CC.
void WorldUpdate(u8* world, int unused, f32 dt) {
    u8* obj = NULL;
    if (*(s32*)(world + 0x28) == 2) obj = *(u8**)(gGame + 0xD614);
    if (*(s32*)(lbl_803C4298 + 0x18) != 5) return;
    if (!(obj == NULL || *(s32*)(obj + 0xa00) != 0)) return;

    u8* hud = *(u8**)(world + 0x44);
    if (*(u8*)(hud + 0x430) != 0) {
        fn_8001B164(hud);
        EventSend(gEventMgr, *(void**)(*(u8**)(world + 0x44) + 0x434), NULL, 0x307B3C9Fu, 0, 0, 0, 0);
        s32 conv = *(s32*)(world + 0x0);
        if (conv != 0 && *(s32*)*(u8**)(world + 0x44) == 0) {
            HudShowConversation(*(u8**)(world + 0x44), conv);
            *(s32*)(world + 0x30) = *(s32*)(world + 0x0);
            *(s32*)(world + 0x0) = 0;
        }
    }
    if (*(s32*)(*(u8**)(world + 0x44) + 0x42c) != 0) fn_8001B128(*(u8**)(world + 0x44));
    s32 pending = *(s32*)(world + 0x38);
    if (pending != 0 && *(s32*)(world + 0x28) != 4 && *(s32*)(lbl_805FBBE0 + 0xa1c) == 0) {
        HudShowConversation(*(u8**)(world + 0x44), pending);
        *(s32*)(world + 0x38) = 0;
    }
    void* screens = *(void**)(*(u8**)(world + 0x44) + 0x434);
    if (screens != NULL) {
        HudUpdateScreens(screens, dt);
        if (*(s32*)(world + 0x34) != 0) {
            u8* h = *(u8**)(world + 0x44);
            u8* cur = fn_8001B044(h, *(u32*)(world + 0x30));
            if (**(u8***)(h + 0x24) == cur) {
                fn_801BBB68(*(void**)(h + 0x434), *(s32*)(world + 0x34), 0, 0);
                *(s32*)(world + 0x34) = 0;
            }
        }
    }
    fn_80051940(dt);
    SessionUpdateDisabled(gGame + 0x5c8, dt);
    fn_80012CB0(gGame + 0x5c8, dt);
    f32 t = *(f32*)(gGame + 0x7cc);
    if (t > 0.0f) {
        t = t - lbl_805FB918;
        *(f32*)(gGame + 0x7cc) = t;
        if (t <= 0.0f) *(f32*)(gGame + 0x7cc) = 0.0f;
    }
}

// 0x8003DCE0
// Releases the object returned by vtable slot 0xCC (clearing the back reference at +0x268), then
// either notifies the zone resource (flag bit 0 of +0xB8) or decrements the use count of the
// matching entry in the list owned by fn_801F06FC().
void fn_8003DCE0(u8* obj) {
    if (obj == NULL) return;
    typedef u8* (*GetFn)(u8*);
    u8* h = (*(GetFn*)(*(u8**)obj + 0xCC))(obj);
    if (h != NULL) {
        *(s32*)(obj + 0x268) = 0;
        fn_8020E864(h, 1);
    }
    if (*(u32*)(obj + 0xB8) & 1) {
        u8* world = *(u8**)gWorldState;
        fn_801DB398(*(u8**)(world + 0x1A4) + gZoneResourceSize + 0xE4, 0x2C733C94u, obj);
        return;
    }
    u8* list = **(u8***)((u8*)fn_801F06FC() + 0x8);
    for (s32 i = 0; i < *(s32*)(list + 0x48); i++) {
        u8** tbl = *(u8***)(list + 0x44);
        if (tbl[i] != NULL) {
            u8* link = *(u8**)(obj + 0x9C);
            u32 key = link != NULL ? *(u32*)(link + 0x8) : 0;
            u8* ent = tbl[i];
            if (*(u32*)(ent + 0x4) == key && *(s32*)ent > 0) {
                *(s32*)ent = *(s32*)ent - 1;
            }
        }
    }
}

// 0x8003DDE8
// Looks up the geometry/prop entry for a part kind: builds the part key + "_geo" (+ "_lr"),
// finds it through CharacterFind (bo00 parts, "_propgroup1") or fn_8003DB78. When missing,
// logs the key and falls back to a hard-coded hash per kind (0..4) for fn_8003DB78.
void* fn_8003DDE8(void* unused, s32 kind, s32 arg2, s32 lowRes, s32 arg4) {
    char key[0x80];
    char msg[0x108];
    u32 hash;
    void* r;
    PartAnimKey(key, kind, arg2);
    if (lowRes != 0) strcat(key, "_lr");
    strcat(key, "_geo");
    if (fn_80338BC0(key, "bo00") != NULL) {
        strcat(key, "_propgroup1");
        hash = StringHash(key);
        r = CharacterFind((s32)hash);
        lbl_803C4E28[lbl_805FB034++] = 1;
        lbl_805FB030[0] = 1;
    } else {
        strcat(key, "A");
        hash = StringHash(key);
        r = fn_8003DB78(hash, arg4, lowRes);
    }
    if (r == NULL) {
        SysSprintf(msg, "The part list is missing a part it should have - %s\n", key);
        switch (kind) {
        case 0: hash = lowRes ? 0xE1EA69ADu : 0x4473EF72u; break;  // p_hd_sha001[_lr]_geoA
        case 1: hash = lowRes ? 0x00ECE0A0u : 0xB0FA15BCu; break;  // p_lg_ski001[_lr]_geoA
        case 2: hash = lowRes ? 0x4E809A74u : 0x3743614Fu; break;  // p_la_fee001[_lr]_geoA
        case 3: hash = lowRes ? 0x6E9930BBu : 0xB3D7695Du; break;  // p_ra_gun003[_lr]_geoA
        case 4: hash = lowRes ? 0xAF41A96Cu : 0xA39A0B94u; break;  // p_to_bul001[_lr]_geoA
        default: break;
        }
        r = fn_8003DB78(hash, arg4, lowRes);
    }
    return r;
}

// 0x800429C0
// Collects every UI element flagged 2 (+0x64) from the elements of both screen sets into the
// table lbl_803C4F60 (0x96 entries), calling its virtual slot 0x30(2) on each, and zeroes the rest.
void fn_800429C0(void) {
    s32 count = 0;
    void** out = lbl_803C4F60;
    for (s32 zone = 0; zone < 2; zone++) {
        u8* set = (u8*)ScreenSetsFind(gScreenSets, zone);
        if (set == NULL) continue;
        for (s32 i = 0; i < *(s32*)(set + 0x60); i++) {
            u8* grp = *(u8**)(set + 0x40 + i * 4);
            if (grp == NULL) continue;
            for (s32 j = 0; j < *(s32*)(grp + 0x13C); j++) {
                u8* el = *(u8**)(*(u8**)(grp + 0x140) + j * 4);
                if (*(u32*)(el + 0x64) & 2) {
                    *out = el;
                    typedef void (*Fn)(u8*, s32);
                    (*(Fn*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
                    out++;
                    count++;
                }
            }
        }
    }
    for (s32 i = count; i < 0x96; i++) lbl_803C4F60[i] = NULL;
}

// 0x80043D84
// Steps the two-phase counter in the record at +0xD4 (mode at +0x18: 1 uses the counter at
// +0x10 counting 0..15, -1 uses +0x14 counting 0..3, 0 idle). dir 0 steps forward, else back;
// running past an end switches phase. Stores the new mode/counters.
void fn_80043D84(u8* obj, s32 dir) {
    u8* r = *(u8**)(obj + 0xD4);
    s32 mode = *(s32*)(r + 0x18);
    s32 cur;
    if (mode == 1) cur = *(s32*)(r + 0x10);
    else if (mode == -1) cur = *(s32*)(r + 0x14);
    else cur = -1;
    if (dir == 0) {
        cur++;
        if (mode == 1) {
            if (cur >= 0x10) { cur = 0; mode = -1; }
        } else if (mode == -1) {
            if (cur >= 4) { cur = -1; mode = 0; }
        } else {
            cur = 0;
            mode = 1;
        }
    } else {
        cur--;
        if (mode == 1) {
            if (cur < 0) { cur = -1; mode = 0; }
        } else if (mode == -1) {
            if (cur < 0) { cur = 0xF; mode = 1; }
        } else {
            cur = 3;
            mode = -1;
        }
    }
    if (mode == 1) {
        *(s32*)(r + 0x18) = mode;
        *(s32*)(r + 0x10) = cur;
        *(s32*)(r + 0x14) = -1;
    } else if (mode == -1) {
        *(s32*)(r + 0x18) = mode;
        *(s32*)(r + 0x14) = cur;
        *(s32*)(r + 0x10) = -1;
    } else {
        *(s32*)(r + 0x14) = -1;
        *(s32*)(r + 0x10) = -1;
        *(s32*)(r + 0x18) = 0;
    }
}

// 0x8004D968
// Sets up a monster from a preset index: attack timer (one of two constants depending on flag
// 0x4000 of the class record), builds the preset into the monster at +0x898, sets the display
// name from the preset's name hash (row +4), and for flag 0x200000 returns fn_8004DAF8(obj, 0, arg2).
void* fn_8004D968(u8* obj, s32 preset, s32 arg2) {
    if (!(*(u32*)(*(u8**)(obj + 0x4) + 0x1C) & 0x4000)) {
        *(f32*)(obj + 0x9F4) = lbl_805FD25C;
    } else {
        *(f32*)(obj + 0x9F4) = lbl_805FD268;
    }
    MonsterBuildPreset(obj + 0x898, preset);
    *(u8*)(obj + 0xC84) = 1;
    u8* spec = *(u8**)(gGameDb->data + 0x9180) + preset * 0x48;
    s32 id = TextIdFromHash(*(u32*)(spec + 0x4));
    const char* name;
    if (id == -1) {
        name = lbl_80354800;
    } else if (((u32)id & 0x80000000u) && gStringTable2 != NULL) {
        u8* t = (u8*)gStringTable2;
        const char** arr = *(const char***)t;
        u32 idx = (u32)id & 0x7FFFFFFFu;
        if (arr != NULL && idx < *(u32*)(t + 0xC)) name = arr[idx];
        else name = lbl_805F8200;
    } else {
        u8* t = (u8*)gStringTable;
        const char** arr = *(const char***)t;
        if (arr != NULL && (u32)id < *(u32*)(t + 0xC)) {
            name = arr[(u32)id];
        } else {
            name = lbl_805F8200;
            if (id == -1) name = lbl_8035480C;   // unreachable: id == -1 handled above
        }
    }
    if (name != NULL) MonsterSetName(obj + 0x898, name);
    if (*(u32*)(*(u8**)(obj + 0x4) + 0x20) & 0x200000) return fn_8004DAF8(obj, 0, arg2);
    return NULL;
}

// True when the movie player exists, is active (lbl_805FB970 and lbl_805FB974) and its virtual
// slot 0x34 reports ready.
static BOOL MoviePlayerReady(void) {
    if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL) {
        typedef BOOL (*Fn)(void*);
        return (*(Fn*)(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0;
    }
    return FALSE;
}

// 0x800098F4
// Per-frame game update. Flag bit 1 of +0x3C selects a reduced update (just fn_801214D8 and the
// movie-player watchdog that fires fn_801F2C10 after 30 idle frames); otherwise ticks the loaded
// zones, the world and the sub-systems (the zone/effect subsystems only when +0x4C is 0).
void fn_800098F4(u8* g, f32 f1, f32 dt) {
    fn_800E6360(dt);
    if (!((*(u32*)(g + 0x3C) >> 1) & 1)) {
        if (*(u8*)(g + 0x4C) == 0) {
            lbl_805FAE64 = 0.0f;
            lbl_805FB314 = 0.0f;
            for (s32 z = 0; z <= *(s32*)(*(u8**)gWorldState + 0x4); z++) {
                if (ZoneIsLoaded(gWorldState, z)) {
                    u8* zr = *(u8**)(*(u8**)gWorldState + 0x1A4) + z * gZoneResourceSize;
                    u8* arr = *(u8**)(zr + 0xA0);
                    for (s32 k = 0; k < *(u8*)(zr + 0xA4); k++) fn_8010A76C(arr + k * 0x4C, dt);
                }
            }
        }
        WorldTick(gWorldState, dt);
        fn_8003CF7C(dt);
        fn_8012570C(dt);
        u8* mini = lbl_805FB134;
        if (mini != NULL && lbl_805FB130 == 0) {
            typedef void (*Fn)(u8*, f32);
            (*(Fn*)(*(u8**)(mini + 0x4) + 0x20))(mini, dt);
        }
        if (gObjDb != NULL) fn_800D3818((u8*)gObjDb, dt);
        fn_800D6E14(dt);
        if (*(u8*)(g + 0x4C) == 0) {
            if (!((*(u32*)(g + 0x3C) >> 2) & 1)) {
                fn_801CE7FC(lbl_8047C760, dt);
                fn_801BE44C(lbl_8047C790, dt);
                fn_801C3C2C(lbl_8047C7AC, dt);
                fn_8010A678();
            }
            fn_801B420C(lbl_8047C7D8);
            fn_801D8BB4(lbl_8047C640, dt);
        } else {
            fn_801214D8(lbl_803C2A20 + 0x10, f1, dt);
        }
        ZoneEffectsUpdate(lbl_803C4298 + 0x40, dt);
    } else {
        fn_801214D8(lbl_803C2A20 + 0x10, f1, dt);
        if (MoviePlayerReady()) {
            if (*(s32*)(g + 0x13C) > 0 && lbl_805FB9D0 != 0) {
                lbl_805FAE68 = lbl_805FAE68 + 1;
                if (lbl_805FAE68 > 0x1E) fn_801F2C10(0xFFFA0400u);
            } else {
                lbl_805FAE68 = 0;
            }
        }
    }
    fn_801E28DC(lbl_805FB8A8, dt);
}

// 0x8000B0E4
// Pushes a 0x18-byte command record onto the ring buffer (head +0x5C, tail +0x60, capacity +0x64,
// records at +0x68) when the movie player is ready; otherwise hands it to fn_801DFB58(cmd, 2).
// On success switches the state at +0x18 to 8 (flag +0x50). Returns 1 when queued.
s32 fn_8000B0E4(u8* q, u8* cmd) {
    s32 queued = 0;
    if (cmd == NULL) return 0;
    if (MoviePlayerReady()) {
        if (fn_801F5220()) {
            s32 pushed = 0;
            if (cmd != NULL) {
                u8* rec;
                s32 head = *(s32*)(q + 0x5C);
                s32 next = (*(s32*)(q + 0x60) + 1) % *(s32*)(q + 0x64);
                if (head == next) {
                    rec = NULL;  // full
                } else {
                    *(s32*)(q + 0x60) = next;
                    if (head < 0) {
                        *(s32*)(q + 0x5C) = next;
                        *(s32*)(q + 0x54) = 1;
                    }
                    s32 idx = *(s32*)(q + 0x60);
                    s32 ok;
                    if (idx < 0 || idx >= *(s32*)(q + 0x64)) {
                        ok = 0;
                    } else {
                        s32 cap = *(s32*)(q + 0x64);
                        s32 h = *(s32*)(q + 0x5C);
                        s32 lo = idx;
                        s32 hi = idx;
                        if (idx < h) {
                            lo = idx + cap;
                            hi = idx + cap;
                        }
                        ok = (h <= lo && lo <= hi);
                    }
                    rec = ok ? *(u8**)(q + 0x68) + idx * 0x18 : NULL;
                }
                if (rec != NULL) {
                    pushed = 1;
                    memcpy(rec, cmd, 0x18);
                } else {
                    GameLog(4, 0, "QueueCommand failed.\n");
                }
            }
            if (pushed != 0) {
                if (*(s32*)(q + 0x18) != 8) {
                    *(s32*)(q + 0x18) = 8;
                    *(s32*)(q + 0x50) = 1;
                }
                queued = 1;
            }
        } else {
            fn_801DFB58(cmd, 2);
        }
    }
    return queued;
}

// 0x80013340
// Snapshots the game state (copies the two 0x100-byte blocks, resets counters) and then walks the
// zone slots (plus the extra slot after the last) looking for the first zone-resource entity
// that RegisterMovingCharacter() rejects; slots of type 6 name the zone to scan (+8).
void fn_80013340(u8* g) {
    fn_800126A0(g + 0xC5C, gWorldState, g + 0xD00, 0x258);
    *(s32*)(g + 0xA18) = 1;
    memcpy(g + 0xA1C, g, 0x100);
    memcpy(g + 0xB1C, g + 0x100, 0x100);
    *(s32*)(g + 0xA18) = 1;
    fn_800D46B0(gObjDb);
    memset(g + 0xA580, 0, 0x50);
    *(s32*)(g + 0xA14) = 0;
    s32 off = 0;
    for (s32 i = 0; i < ZoneSlotCapacity() + 1; i++, off += 0x30) {
        s32 zone = -1;
        if (i == ZoneSlotCapacity()) {
            zone = 0;
        } else {
            u8* slot = *(u8**)(gWorldState + 0xB0) + off;
            if (*(s32*)(slot + 0x4) == 6) zone = *(s32*)(slot + 0x8);
        }
        if (zone < 0) continue;
        s32 found = 0;
        u8* zr0 = *(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize;
        s32 total = *(s32*)(zr0 + 0x94) + *(s32*)(zr0 + 0x98);
        for (s32 j = 0; j < total; j++) {
            u8* rec;
            u8* item;
            ZoneEntity(*(u8**)(*(u8**)gWorldState + 0x1A4) + zone * gZoneResourceSize, j, &rec, &item);
            if (item != NULL && RegisterMovingCharacter(item) == 0) {
                found = 1;
                break;
            }
        }
        if (found) return;
    }
}

// 0x80013D54
// Resets the 4-field setting record and feeds the three tuning values (0.4, 0.5, 0.7) to the
// audio-ish setters, then blends the world values at +0x1A84/+0x1A88 (clamped to 1.0..1.4 and
// -0.15..0.15) with 1.4/-0.15 or 1.0/0.15 into the two floats at +4/+8 of two fn_8015EC9C slots.
void fn_80013D54(u8* r) {
    *(u8*)(r + 0x10) = 0;
    *(u8*)(r + 0x11) = 0;
    *(s32*)(r + 0x0) = 4;
    fn_8020B024((f32)4 / 10.0f);
    *(s32*)(r + 0x8) = 5;
    fn_8020B078((f32)5 / 10.0f);
    *(s32*)(r + 0x4) = 7;
    fn_8020B160((f32)7 / 10.0f);
    *(s32*)(r + 0xC) = 5;
    f32 t = (f32)5 / 10.0f;
    u8* w = *(u8**)gWorldState;
    f32 x = *(f32*)(w + 0x1A84);
    f32 hi;
    if (x < 1.0f) hi = 1.0f;
    else if (x > 1.4f) hi = 1.4f;
    else hi = x;
    f32 y = *(f32*)(w + 0x1A88);
    f32 lo;
    if (y < -0.15f) lo = -0.15f;
    else if (y > 0.15f) lo = 0.15f;
    else lo = y;
    if (t < 0.5f) {
        f32 u = t / 0.5f;
        f32 v = 1.4f + u * (hi - 1.4f);
        *(f32*)(fn_8015EC9C(lbl_8040D500, 1.4f, u) + 0x4) = v;
        u = t / 0.5f;
        v = -0.15f + u * (lo - -0.15f);
        *(f32*)(fn_8015EC9C(lbl_8040D500, -0.15f, u) + 0x8) = v;
    } else {
        f32 u = (t - 0.5f) / 0.5f;
        f32 v = hi + u * (1.0f - hi);
        *(f32*)(fn_8015EC9C(lbl_8040D500, u, 0.5f) + 0x4) = v;
        u = (t - 0.5f) / 0.5f;
        v = lo + u * (0.15f - lo);
        *(f32*)(fn_8015EC9C(lbl_8040D500, u, 0.5f) + 0x8) = v;
    }
}

// 0x800195A4
// Registers the camera script callbacks (names are the callback identifiers).
void fn_800195A4(void) {
    RegisterScriptFunc("acquire_camera_semaphore", 0x4E, (void*)acquire_camera_semaphore);
    RegisterScriptFunc("release_camera_semaphore", 0x4E, (void*)release_camera_semaphore);
    RegisterScriptFunc("set_igc_mode", 0x4E, (void*)set_igc_mode);
    RegisterScriptFunc("set_igc_mode_from_sensor_param", 0x4E, (void*)set_igc_mode_from_sensor_param);
    RegisterScriptFunc("clear_igc_mode", 0x4E, (void*)clear_igc_mode);
    RegisterScriptFunc("SetCameraMode", 0x4E, (void*)SetCameraMode);
    RegisterScriptFunc("set_camera_mode_from_sensor_type", 0x4E, (void*)set_camera_mode_from_sensor_type);
    RegisterScriptFunc("clear_camera_mode", 0x4E, (void*)clear_camera_mode);
    RegisterScriptFunc("set_camera_location", 0x4E, (void*)set_camera_location);
    RegisterScriptFunc("end_trigger_camera", 0x4E, (void*)end_trigger_camera);
    RegisterScriptFunc("send_event", 0x4E, (void*)send_event);
    RegisterScriptFunc("send_request", 0x4E, (void*)send_request);
    RegisterScriptFunc("send_request_complete", 0x4E, (void*)send_request_complete);
    RegisterScriptFunc("ping", 0x4E, (void*)ping);
    RegisterScriptFunc("broadcast_on_done_event", 0x4E, (void*)broadcast_on_done_event);
    RegisterScriptFunc("set_camera_location_from_sensor_param", 0x4E, (void*)set_camera_location_from_sensor_param);
    RegisterScriptFunc("set_timeout_from_sensor_param", 0x4E, (void*)set_timeout_from_sensor_param);
    RegisterScriptFunc("should_igc_abort", 0x1D, (void*)should_igc_abort);
    RegisterScriptFunc("special_camera_done_slerping", 0x1D, (void*)special_camera_done_slerping);
    RegisterScriptFunc("set_interp_timer", 0x4E, (void*)set_interp_timer);
    RegisterScriptFunc("fixed_camera_return", 0x4E, (void*)fixed_camera_return);
    RegisterScriptFunc("force_fixed_camera_return", 0x4E, (void*)force_fixed_camera_return);
}

// 0x8001D104
// Returns 0 when the currently selected part (from the list at +0x8+idx*0x1A4, index into the
// save block's part table +0xA58) is already equipped in one of the slots +0x944/+0x948/
// +0x94C/+0x950/+0x954 (compared by id at +0xC); returns 1 when the owning monster already has
// it, else whether fn_8021EE94 says the part is not usable (== 0).
s32 fn_8001D104(u8* g) {
    u8* list = g + *(s32*)(g + 0xA80) * 0x1A4 + 0x8;
    s32 count = *(s32*)(list + 0x198);
    s32 sel = (count > 0 && *(s32*)(list + 0x190) < count) ? *(s32*)(list + *(s32*)(list + 0x190) * 4) : -1;
    if (sel < 0) return 1;
    sel = (count > 0 && *(s32*)(list + 0x190) < count) ? *(s32*)(list + *(s32*)(list + 0x190) * 4) : -1;
    u8* save = lbl_805FBBE0;
    u8* part = *(u8**)(save + sel * 4 + 0xA58);
    u32 id = *(u32*)(part + 0xC);
    u8* slot;
    slot = *(u8**)(g + 0x944);
    if (slot != NULL && *(u32*)(slot + 0xC) == id) return 0;
    slot = *(u8**)(g + 0x948);
    if (slot != NULL && *(u32*)(slot + 0xC) == id) return 0;
    slot = *(u8**)(g + 0x94C);
    if (slot != NULL && *(u32*)(slot + 0xC) == id) return 0;
    slot = *(u8**)(g + 0x950);
    if (slot != NULL && *(u32*)(slot + 0xC) == id) return 0;
    slot = *(u8**)(g + 0x954);
    if (slot != NULL && *(u32*)(slot + 0xC) == id) return 0;
    s32 cur = *(s32*)(lbl_805FAF78 + 0xAD8);
    if (cur != -1) {
        u8* mon = *(u8**)(save + cur * 4 + 0xA30);
        if (mon != NULL && MonsterHasPart(mon, (const PartInstance*)part)) return 1;
    }
    return fn_8021EE94(part) == 0;
}

// 0x80023044
// Occasional ambient sound for the experiment object: when it is active (lbl_805FB134 with tag 0)
// and its value at +0x6C is at least 2.0 and none of the four handles at lbl_80355940 is
// still playing, there is a 67 percent chance to start a sound into handle slot 3.
void fn_80023044(void) {
    u8* obj = lbl_805FB134;
    if (obj != NULL && *(s32*)obj != 0) obj = NULL;
    f32 v = obj != NULL ? *(f32*)(obj + 0x6C) : 0.0f;
    s32 idle;
    if (v < 2.0f) {
        idle = 0;
    } else {
        idle = 1;
        for (s32 k = 0; k < 4; k++) {
            if (fn_8020A9A8(*(s32*)(lbl_80355940 + k * 4)) != 0) {
                idle = 0;
                break;
            }
        }
    }
    if (idle) {
        u32 r = RandomU32();
        if ((f32)(s32)(r % 100 + 1) < 67.0f) {
            *(s32*)(lbl_80355940 + 0xC) = PlaySound(0xFF3FB78Eu, 0, 0, 0, 0, 0);
        }
    }
}

// 0x8002FE14
// Counts a result (kind 2/3/4: counters +0xF64/+0xF68/+0xF6C, total +0xF60), plays its sound
// and calls virtual slot 0x60, then updates the progress bar element (+0x280) and the
// remaining-count text (+0x278): bar value = min(total / 10 * max, max) where max is at +0xF48.
void fn_8002FE14(u8* obj, s32 kind) {
    typedef void (*Fn)(u8*);
    switch (kind) {
    case 2:
        *(s32*)(obj + 0xF64) = *(s32*)(obj + 0xF64) + 1;
        *(s32*)(obj + 0xF60) = *(s32*)(obj + 0xF60) + 1;
        PlaySound(0x37C87E25u, 0, 0, 0, 0, 0);
        (*(Fn*)(*(u8**)(obj + 0x4) + 0x60))(obj);
        break;
    case 3:
        *(s32*)(obj + 0xF68) = *(s32*)(obj + 0xF68) + 1;
        *(s32*)(obj + 0xF60) = *(s32*)(obj + 0xF60) + 1;
        PlaySound(0x7D7672C9u, 0, 0, 0, 0, 0);
        (*(Fn*)(*(u8**)(obj + 0x4) + 0x60))(obj);
        break;
    case 4:
        *(s32*)(obj + 0xF6C) = *(s32*)(obj + 0xF6C) + 1;
        *(s32*)(obj + 0xF60) = *(s32*)(obj + 0xF60) + 1;
        PlaySound(0x9FC3AA91u, 0, 0, 0, 0, 0);
        (*(Fn*)(*(u8**)(obj + 0x4) + 0x60))(obj);
        break;
    default:
        break;
    }
    f32 max = *(f32*)(obj + 0xF48);
    f32 val = max * ((f32)*(s32*)(obj + 0xF60) / 10.0f);
    if (!(val < max)) val = max;
    u8* bar = *(u8**)(obj + 0x280);
    *(f32*)(bar + 0x74) = val;
    typedef void (*BarFn)(u8*, s32);
    (*(BarFn*)(*(u8**)(bar + 0x138) + 0x2C))(bar, 0x1A);
    UIText_SetString_Int(**(u32**)(obj + 0x278), 10 - *(s32*)(obj + 0xF60));
}

// 0x80039440
// Picks a random chest layout (1..3, stored at +0x5E8), then for each of the 4 regions grows the
// bounding box (+0x604..+0x610 of the 0x28-byte record at +0x5EC+i*0x28 / +i*0x28) over its four
// "Marker<chest>_<n>" elements (offset by the "Box_<chest>_Group" element), stores the region
// element at +0x69C+i*4, sets its texture from the "<..>_TextureList" and shows it.
void fn_80039440(u8* obj) {
    static const s32 kMarkers[4][4] = {{0, 8, 1, 4}, {4, 1, 7, 2}, {3, 4, 2, 6}, {5, 0, 4, 3}};
    char buf[0x40];
    *(s32*)(obj + 0x5E8) = (s32)(RandomU32() % 3) + 1;
    void* set = ScreenSetsCurrent(gScreenSets);
    SysSprintf(buf, "Box_%d_Group", *(s32*)(obj + 0x5E8));
    u8* group = (u8*)ScreenSetFindElement(set, StringHash(buf));
    typedef void (*ElFn)(u8*, s32);
    (*(ElFn*)(*(u8**)(group + 0x138) + 0x2C))(group, 0x1A);
    for (s32 i = 0; i < 4; i++) {
        for (s32 j = 0; j < 4; j++) {
            SysSprintf(buf, "Marker%d_%d", *(s32*)(obj + 0x5E8), kMarkers[i][j]);
            u8* el = (u8*)ScreenSetFindElement(set, StringHash(buf));
            if (el != NULL) {
                f32 pt[3];
                pt[0] = *(f32*)(el + 0x68) + *(f32*)(group + 0x68);
                pt[1] = *(f32*)(el + 0x6C) + *(f32*)(group + 0x6C);
                pt[2] = *(f32*)(el + 0x70) + *(f32*)(group + 0x70);
                u8* rec = obj + i * 0x28;
                f32 cur = *(f32*)(rec + 0x60C);
                *(f32*)(rec + 0x60C) = (pt[0] <= cur) ? cur : pt[0];
                cur = *(f32*)(rec + 0x604);
                *(f32*)(rec + 0x604) = (pt[0] >= cur) ? cur : pt[0];
                cur = *(f32*)(rec + 0x610);
                *(f32*)(rec + 0x610) = (pt[1] <= cur) ? cur : pt[1];
                cur = *(f32*)(rec + 0x608);
                *(f32*)(rec + 0x608) = (pt[1] >= cur) ? cur : pt[1];
                fn_800311E8(obj + 0x5EC + i * 0x28, pt);
            }
        }
        SysSprintf(buf, "Chest_%d_Region%d", *(s32*)(obj + 0x5E8), i);
        u8* region = (u8*)ScreenSetFindElement(set, StringHash(buf));
        *(u8**)(obj + 0x69C + i * 4) = region;
        SysSprintf(buf, "Chest_%d_Region%d_TextureList", *(s32*)(obj + 0x5E8), i);
        ScriptValue tex = UITextureList_GetTexture(StringHash(buf), 0);
        UIImage_SetTexture(**(u32**)(obj + 0x69C + i * 4), (BOOL)tex);
        u8* el = *(u8**)(obj + 0x69C + i * 4);
        (*(ElFn*)(*(u8**)(el + 0x138) + 0x2C))(el, 2);
    }
}

// 0x8003D580
// Resets the combat part-load bookkeeping, then restores every part of the two combatants to
// full health (the second record only in online battles) and continues in fn_8003E000. When there
// is no current battle (indices at gGame+0xD62C/0xD630 negative) it builds a preset monster 4
// (field +0x44 = 6) to compare against the saved one, or calls fn_8003D964 without a saved monster.
void fn_8003D580(u8* arg) {
    lbl_805FB01C = 0;
    lbl_805FB020 = 0;
    lbl_805FB030[0] = 0;
    lbl_805FB038 = 10;
    for (s32 i = 0; i < 10; i++) lbl_803C4E28[i] = 0;
    lbl_805FB034 = 0;
    lbl_805FB025 = 1;
    s32 idxA = *(s32*)(gGame + 0xD62C);
    s32 idxB = *(s32*)(gGame + 0xD630);
    if (idxA >= 0 && idxB >= 0) {
        u8* recB = gGame + 0xAB98 + idxB * 0x13C;
        PartInstance* torso = *(PartInstance**)(recB + 0x14);
        u8* recA = gGame + 0xAB98 + idxA * 0x13C;
        if (!IsOnlineBattle()) {
            if (torso == NULL) {
                ZoneLog(gIdPool, "Inventory Torso Missing!!!!\n");
            } else if (PartGetCurrentHealth(torso) <= 0) {
                ZoneLog(gIdPool, "Inventory Torso HP < 0!!!!\n");
                ZoneLog(gIdPool, "Inventory Torso Max HP %d\n", PartGetMaxHealth(torso));
                ZoneLog(gIdPool, "Inventory Torso Quality %d\n", (s32)PartGetQuality(torso));
            }
        }
        for (s32 k = 0; k < 5; k++) {
            PartInstance* p = *(PartInstance**)(recB + 0x4 + k * 4);
            if (p != NULL) PartSetHealth(p, PartGetMaxHealth(p));
            if (IsOnlineBattle()) {
                PartInstance* q = *(PartInstance**)(recA + 0x4 + k * 4);
                if (q != NULL) PartSetHealth(q, PartGetMaxHealth(q));
            }
        }
        fn_8003E000(arg, recA, recB);
        return;
    }
    if (SaveGetCurrentMonster(lbl_805FBBE0) == NULL) {
        fn_8003D964(arg);
        return;
    }
    u8 mon[0x68];
    MonsterCtor(mon);
    MonsterBuildPreset(mon, 4);
    *(s32*)(mon + 0x44) = 6;
    fn_8003E000(arg, SaveGetCurrentMonster(lbl_805FBBE0), mon);
    MonsterDtor(mon, -1);
}

// 0x80020334
// Runs the per-type update on every object in the list (count at +0, object pointers at +0x10):
// the object's type (+0x48, 0..9) selects one of ten handlers.
void fn_80020334(u8* list) {
    for (s32 i = 0; i < *(s32*)list; i++) {
        u8* obj = *(u8**)(list + 0x10 + i * 4);
        if (obj == NULL) continue;
        switch (*(u32*)(obj + 0x48)) {
        case 0: fn_80020590(obj); break;
        case 1: fn_80020480(obj); break;
        case 2: fn_80020758(obj); break;
        case 3: fn_80020864(obj); break;
        case 4: fn_8002104C(obj); break;
        case 5: fn_8002112C(obj); break;
        case 6: fn_80021240(obj); break;
        case 7: fn_80020A04(obj); break;
        case 8: fn_80020D28(obj); break;
        case 9: fn_80021354(obj); break;
        default: break;
        }
    }
}

// Copies a 0x28-byte spawn record field by field (the byte at +0x20 only, not the padding).
static void CopySpawnRecord(u8* dst, const u8* src) {
    *(u32*)(dst + 0x00) = *(const u32*)(src + 0x00);
    *(u32*)(dst + 0x04) = *(const u32*)(src + 0x04);
    *(u32*)(dst + 0x08) = *(const u32*)(src + 0x08);
    *(f32*)(dst + 0x0C) = *(const f32*)(src + 0x0C);
    *(f32*)(dst + 0x10) = *(const f32*)(src + 0x10);
    *(f32*)(dst + 0x14) = *(const f32*)(src + 0x14);
    *(u32*)(dst + 0x18) = *(const u32*)(src + 0x18);
    *(u32*)(dst + 0x1C) = *(const u32*)(src + 0x1C);
    *(u8*)(dst + 0x20) = *(const u8*)(src + 0x20);
    *(u32*)(dst + 0x24) = *(const u32*)(src + 0x24);
}

// 0x800D70EC
// Upserts a 0x28-byte record into the table at lbl_805FB188 (count at +0x320, max 20): replaces
// the entry whose first word matches, else appends. The float argument is not used.
void fn_800D70EC(s32* rec, f32 unused) {
    u8* tbl = lbl_805FB188;
    if (tbl == NULL) return;
    u32 key = *(u32*)rec;
    s32 count = *(s32*)(tbl + 0x320);
    s32 found = -1;
    for (s32 i = 0; i < count; i++) {
        if (*(u32*)(tbl + i * 0x28) == key) {
            found = i;
            break;
        }
    }
    if (found >= 0) {
        CopySpawnRecord(lbl_805FB188 + found * 0x28, (u8*)rec);
        return;
    }
    u8* t = lbl_805FB188;
    if (*(s32*)(t + 0x320) >= 0x14) return;
    s32 n = *(s32*)(t + 0x320);
    CopySpawnRecord(t + n * 0x28, (u8*)rec);
    *(s32*)(t + 0x320) = *(s32*)(t + 0x320) + 1;
}

}  // extern "C"
