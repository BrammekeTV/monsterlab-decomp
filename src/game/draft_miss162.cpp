// Misc functions: RandomU32 (Mersenne Twister), PlaySound, music selection, UI/effect helpers.
#include "types.h"
#include "engine/rend.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
u8*   GetGlobalSettings(void);                                    // 0x80143A04
u32   GetRandomSeed(void);
s32   PlaySoundAt(void* sound, int a, const void* pos, int b);    // 0x80209ABC
s32   ZoneIsLoaded(void* world, s32 zone);
void* fn_801D40E0(s32 n);                                         // 0x801D40E0
void  fn_8008CE28(u8* vec, f32 v);
void  fn_8020B068(s32 a3);                                        // 0x8020B068
void  SpawnEffectAt(void* fxMgr, u32 effect, const f32 posAndScale[4], u8** owner);  // 0x801C80E0
void  fn_801C849C(u8* mgr, u8* item);                             // 0x801C849C
void  fn_801C9DF0(u8* obj, const f32* v, f32* out, f32 s);        // 0x801C9DF0
void  fn_801CA30C(u8* obj, const f32* v);                         // 0x801CA30C
extern u8 lbl_8047C760[];           // 0x8047C760 (effect manager)
void  fn_8016A64C(void* sys, f32 dt);                             // 0x8016A64C
void  fn_8016A3F8(u8* p);                                         // 0x8016A3F8
void  fn_8016A43C(u8* p);                                         // 0x8016A43C
void  fn_8016A788(u8* p);                                         // 0x8016A788
s32   fn_8016AFE8(u8* a, u8* b, u8* c, f32 t);                    // 0x8016AFE8
s32   fn_8016ACB8(u8* a, s32 b, u8* c, s32 d, u8* e, s32 f, s32 g, f32 t);  // 0x8016ACB8
void  fn_8017387C(f32* out, u8* obj);                             // 0x8017387C
void  fn_80173944(u8* obj);                                       // 0x80173944
void  fn_8018C508(u8* p, f32 dt);                                 // 0x8018C508
extern u8  lbl_8047C640[];          // 0x8047C640 (camera)
u32   StringHash(const char* s);                                  // 0x801EB704
void* UIFindElement(u32 hash);                                    // 0x80195464
void* UIWidgetFindChild(void* widget, u32 hash);                  // 0x80199AEC
u8*   fn_8019BF64(u8* widget, u32 hash);                          // 0x8019BF64
s32   fn_8019BC80(u8* w, u32 a);                                  // 0x8019BC80
u8*   fn_801B3308(void);                                          // 0x801B3308
void  fn_801B3310(u8* p, u8* q);                                  // 0x801B3310
void  fn_801B3378(u8* p, u8* key);                                // 0x801B3378
u8* fn_801F61AC(u8* arg0, s32 arg1);                                  // 0x801F61AC
void  fn_8018D2B8(const char* fmt, ...);                          // 0x8018D2B8 (debug log)
void* fn_801DA798(void* p);                                       // 0x801DA798
u8*   fn_8018C39C(u8* p, s32 idx);                                // 0x8018C39C
extern u8  lbl_805FB64C;            // 0x805FB64C
extern u32 lbl_805FB854;            // 0x805FB854 (32-bit frame counter)
void  fn_80142ED0(u8* base, s32 size);                            // 0x80142ED0
void fn_8015E6D0(void* unusedR3, s32 frame, s32 unusedR5, s32 unusedR6, s32 value);                         // 0x8015E6D0
void fn_8015E74C(void* unusedR3, s32 frame, s32 unusedR5, s32 unusedR6, s32 value);                         // 0x8015E74C
void* ScreenSetsCurrent(void* sets);                              // 0x8018D1E4
void* ScreenSetFindWidget(void* set, u32 hash);                   // 0x80198E34
void  FriendListHideFloaty(s32 a);
void  fn_8013AFF0(void);                                          // 0x8013AFF0
s32   fn_802E64A0(u8* p, s32 n);                                  // 0x802E64A0
void  fn_8023A5B0(u8* p);                                         // 0x8023A5B0
void  fn_8023A68C(u8* p);                                         // 0x8023A68C
int   strcmp(const char* a, const char* b);
extern s32  lbl_803F0E30[];         // 0x803F0E30 (friend display order, 64 entries)
extern u32  lbl_803F0F30[];         // 0x803F0F30 (per-friend flags/sort keys)
extern u8   lbl_80556AB0[];         // 0x80556AB0 (lock)
extern u8   lbl_80556E80[];         // 0x80556E80 (friend records, 0x20 bytes each from +0x340)
extern s32  lbl_805F9028;           // 0x805F9028 (friend list sort mode)
extern s32  lbl_805FB35C;           // 0x805FB35C
extern s32  lbl_805FBB48;           // 0x805FBB48 (lock depth)
void* EventPost(s32 id, void* sender);                            // 0x801F3918
void  BufferFree(void* p);                                        // 0x801D4980
u32   fn_80143974(s32 seed, void* data, u32 len);                 // 0x80143974
void  fn_801F5E48(void* a, s32 b);                                // 0x801F5E48
void  fn_801F5E4C(void* a, s32 b);                                // 0x801F5E4C
s32   fn_801F5A90(u8* pkt);                                       // 0x801F5A90
u32   fn_801F5B94(u8* pkt);                                       // 0x801F5B94
u8*   fn_801F5BA0(u8* pkt);                                       // 0x801F5BA0
void* memcpy(void* d, const void* s, size_t n);
extern u8   gScriptFuncs[];         // 0x80484230 (0xC bytes each: type, fn, hash)
extern u8   lbl_804DD568[];         // 0x804DD568 (packet ring: +8 head, +0xC tail, +0x10 size)
extern u8   lbl_804FB5A8[];         // 0x804FB5A8 (pending receive queue)
extern u8   lbl_804FB5C4[];         // 0x804FB5C4 (pending message)
extern u8   lbl_804FB5E0[];         // 0x804FB5E0 (lock)
extern s32  lbl_80386064[];         // 0x80386064 (thresholds)
extern u8*  lbl_805FB95C;           // 0x805FB95C (current packet)
extern void* lbl_805FB964;          // 0x805FB964 (link object)
extern u32  lbl_805FB96C;           // 0x805FB96C (retry counter)
extern s32  lbl_805FB970;           // 0x805FB970
extern s32  lbl_805FB974;           // 0x805FB974
extern s32  lbl_805FB994;           // 0x805FB994
extern f32  lbl_805FB99C;           // 0x805FB99C
extern s32  lbl_805FB9D8;           // 0x805FB9D8 (lock depth)
extern s32  lbl_805F97F8;           // 0x805F97F8
extern u8   sRendDLs[];             // 0x80401108 (display lists, 0x94 bytes each)
extern u8   sRendDLRecords[];       // 0x80401ADC (records, 0x14 bytes each)
extern u8   lbl_805FB594[];         // 0x805FB594 (render thread state)
extern u8   lbl_803765A0[];         // 0x803765A0 (7-byte texture/copy config entries)
extern s32  lbl_805F9218;           // 0x805F9218
extern u8   lbl_805F9231;           // 0x805F9231
extern u8   lbl_805F9232;           // 0x805F9232
extern s32  lbl_805FB4BC;           // 0x805FB4BC (current display list index)
extern s32  lbl_805FB4C0;           // 0x805FB4C0
extern s32  lbl_805FB4C4;           // 0x805FB4C4
extern s32  lbl_805FB4C8;           // 0x805FB4C8
extern s32  lbl_805FB4CC;           // 0x805FB4CC
extern u8*  lbl_805FB4D0;           // 0x805FB4D0
extern s32  lbl_805FB4D4;           // 0x805FB4D4
extern s32  lbl_805FB4D8;           // 0x805FB4D8
extern s32  lbl_805FB4DC;           // 0x805FB4DC
extern s32  lbl_805FB4E0;           // 0x805FB4E0
extern s32  lbl_805FB4E8;           // 0x805FB4E8
extern s32  lbl_805FB4EC;           // 0x805FB4EC
extern s32  lbl_805FB500;           // 0x805FB500
extern s32  lbl_805FB504;           // 0x805FB504
extern s32  lbl_805FB518;           // 0x805FB518 (frame counter)
extern s32  lbl_805FB51C;           // 0x805FB51C
extern f32 lbl_805F9348;            // 0x805F9348

extern u8   gWorldState[];          // 0x803E6D00
extern u32  lbl_803F6890[];         // 0x803F6890 (Mersenne Twister state, 624 words)
extern s32  lbl_805F91D0;           // 0x805F91D0 (MT index; 0x271 = uninitialised)
extern u32  lbl_805F91D8[];         // 0x805F91D8 (MT mag01 table {0, 0x9908B0DF})
extern s32  lbl_805FB468;           // 0x805FB468 (MT seed, 0 = pick one)
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
extern s32  lbl_8055C578[];         // 0x8055C578 (identity index table, 128 entries)
extern u8   lbl_805FBBB4[];         // 0x805FBBB4
extern f32  lbl_805F81F0;           // 0x805F81F0 (music volume/scale)
}
extern "C++" s32 gZoneResourceSize;  // 0x805FB94C
extern "C++" void* gScreenSets;      // 0x805FB654
extern "C++" void  MusicRequest(const char* name, u8 loop);   // 0x8000FA04
extern "C++" void  fn_8015E654(u32 r3, u32 a4, u32 r5, u32 r6, u32 a7);   // 0x8015E654
extern "C++" void* fn_801953E4(u32 hash);                      // 0x801953E4

typedef s32 (*WidgetVFn)(u8* w, s32 a);
typedef void* (*ObjGetFn)(u8* obj);

extern "C" {

// 0x80143460
// Mersenne Twister (MT19937 variant seeded with the 69069 LCG): returns the next tempered word,
// regenerating the 624-word state when the index runs out.
u32 RandomU32(void) {
    if (lbl_805F91D0 >= 0x270) {
        if (lbl_805F91D0 == 0x271) {
            if (lbl_805FB468 == 0) lbl_805FB468 = GetRandomSeed();
            u32 s = lbl_805FB468;
            for (s32 i = 0; i < 0x270; i++) {
                u32 hi = s & 0xFFFF0000;
                s = s * 0x10DCD + 1;
                lbl_803F6890[i] = hi | (s >> 16);
                s = s * 0x10DCD + 1;
            }
            lbl_805FB468 = s + 1;
        }
        s32 k;
        for (k = 0; k < 0xE3; k++) {
            u32 y = (lbl_803F6890[k] & 0x80000000) | (lbl_803F6890[k + 1] & 0x7FFFFFFF);
            lbl_803F6890[k] = (y >> 1) ^ lbl_803F6890[k + 0x18D] ^ lbl_805F91D8[y & 1];
        }
        for (; k < 0x26F; k++) {
            u32 y = (lbl_803F6890[k] & 0x80000000) | (lbl_803F6890[k + 1] & 0x7FFFFFFF);
            lbl_803F6890[k] = (y >> 1) ^ lbl_803F6890[k - 0xE3] ^ lbl_805F91D8[y & 1];
        }
        u32 y = (lbl_803F6890[0x26F] & 0x80000000) | (lbl_803F6890[0] & 0x7FFFFFFF);
        lbl_805F91D0 = 0;
        lbl_803F6890[0x26F] = lbl_803F6890[0x18C] ^ (y >> 1) ^ lbl_805F91D8[y & 1];
    }
    u32 y = lbl_803F6890[lbl_805F91D0];
    lbl_805F91D0 += 1;
    y ^= y >> 11;
    y ^= (y << 7) & 0x9D2C5680;
    y ^= (y << 15) & 0xEFC60000;
    return y ^ (y >> 18);
}

// 0x80209E70
// Plays the sound with the given id: searches the current zone, then the type-6 entries of the
// world list, then the remaining zones for the sound group, picks a random variation, sets its
// volume byte and plays it. Returns -1 if the sound id is unknown.
s32 PlaySound(u32 hash, s32 a, const void* pos, s32 b, s32 unused, s32 vol) {
    u8* grp = NULL;
    s32 n;
    s32 lim;
    s32 i;
    n = *(s32*)(GetGlobalSettings() + 0x4c);
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
    if (*(s32*)(gWorldState + 0xb4) == 0) n = 0;
    if (ZoneIsLoaded(gWorldState, 0)) {
        u8* zone = *(u8**)(*(u8**)gWorldState + 0x1a4);
        if (zone) {
            grp = *(u8**)(zone + 0x110);
            while (grp && *(u32*)grp != hash) grp = *(u8**)(grp + 0x34);
        }
    }
    u8* ent = *(u8**)(gWorldState + 0xb0);
    for (i = 0; i < n && grp == NULL; i++, ent += 0x30) {
        if (*(s32*)(ent + 4) == 6) {
            grp = *(u8**)(*(u8**)(ent + 0x28) + 0x110);
            while (grp && *(u32*)grp != hash) grp = *(u8**)(grp + 0x34);
        }
    }
    if (grp == NULL) {
        s32 z = *(s32*)(GetGlobalSettings() + 0x6c);
        z = *(s32*)(GetGlobalSettings() + 0x68) + z;
        z += *(s32*)(GetGlobalSettings() + 0x70);
        z += *(s32*)(GetGlobalSettings() + 0x74);
        z += *(s32*)(GetGlobalSettings() + 0x78);
        z += *(s32*)(GetGlobalSettings() + 0x7c);
        z += 1;
        s32 t0 = *(s32*)(GetGlobalSettings() + 0x68);
        s32 t1 = *(s32*)(GetGlobalSettings() + 0x88);
        s32 t2 = *(s32*)(GetGlobalSettings() + 0x6c);
        t1 += t0;
        s32 t3 = *(s32*)(GetGlobalSettings() + 0x70);
        t1 = t2 + t1;
        s32 t4 = *(s32*)(GetGlobalSettings() + 0x74);
        t1 = t3 + t1;
        s32 t5 = *(s32*)(GetGlobalSettings() + 0x78);
        t4 = t4 + t1;
        s32 t6 = *(s32*)(GetGlobalSettings() + 0x7c);
        t5 = t5 + t4;
        s32 t7 = *(s32*)(GetGlobalSettings() + 0x80);
        t5 = t6 + t5;
        lim = *(s32*)(GetGlobalSettings() + 0x84) + (t7 + t5);
        for (; z <= lim && grp == NULL; z++) {
            if (ZoneIsLoaded(gWorldState, z)) {
                u8* zone = *(u8**)(*(u8**)gWorldState + 0x1a4) + z * gZoneResourceSize;
                if (zone) {
                    grp = *(u8**)(zone + 0x110);
                    while (grp && *(u32*)grp != hash) grp = *(u8**)(grp + 0x34);
                }
            }
        }
    }
    if (grp == NULL) return -1;
    u32 cnt = *(u32*)(grp + 0x2c);
    u8* snd;
    if (cnt == 0) {
        snd = NULL;
    } else {
        u32 idx = (u32)((f32)cnt * (2.3283064e-10f * (f32)RandomU32()));
        if (idx == cnt) idx -= 1;
        *(u32*)(grp + 0x30) = idx;
        snd = *(u8**)(grp + 4 + idx * 4);
    }
    if (snd) *(u8*)(snd + 0x2e) = (u8)vol;
    if (lbl_805FBBAC == NULL) {
        void* h = fn_801D40E0(1);
        if (h) {
            for (s32 j = 0; j < 0x80; j++) lbl_8055C578[j] = j;
            for (s32 j = 0; j < 4; j++) {
                u8 v0[12], v1[12], v2[12];
                fn_8008CE28(v0, 0.0f);
                fn_8008CE28(v1, 0.0f);
                fn_8008CE28(v2, 0.0f);
                lbl_805FBBB4[j] = 0;
            }
        }
        lbl_805FBBAC = h;
    }
    return PlaySoundAt(snd, a, pos, b);
}

// 0x8000FBA0
// Script callback: starts the music track (or stinger) for a name hash; scale <= 0 means 1.0.
void app_PlayMusic(u32 a0, f32 a1) {
    s32 flag;
    if (a1 <= 0.0f) a1 = 1.0f;
    lbl_805F81F0 = a1;
    flag = 1;
    switch (a0) {
    case 0x7CFB34EF: MusicRequest("Sounds/Music/Music_Welding1a32kMST", 1); break;
    case 0xEB6B9947: MusicRequest("Sounds/Music/region_music_castle", 1); break;
    case 0x45CEA8D2: MusicRequest("Sounds/Music/castle_music_LightningTower", 1); break;
    case 0x1E14BA78: MusicRequest("Sounds/Music/region_music_mhartiland", 1); break;
    case 0x3FE60C78: MusicRequest("Sounds/Music/region_music_junkyard", 1); break;
    case 0xE1351891: MusicRequest("Sounds/Music/region_music_neverglades", 1); break;
    case 0x25C22B0E: MusicRequest("Sounds/Music/region_music_wildwoods", 1); break;
    case 0x06C4EEAD: MusicRequest("Sounds/Music/MusicProtonSlingshot", 1); break;
    case 0x02DFC379:
    case 0x95F4DBAB: MusicRequest("Sounds/Music/music_experiment_grow_juice", 1); break;
    case 0x74B93DC2: MusicRequest("Sounds/Music/experiment_RobotEvolution", 1); break;
    case 0xA4308887: MusicRequest("Sounds/Music/music_experiment_brain_toss", 1); break;
    case 0xEC22BE9E: MusicRequest("Sounds/Music/experiment_ReactorLeak", 1); break;
    case 0xADA72C85:
    case 0x8D4B9D04: MusicRequest("Sounds/Music/experiment_BuzzSaw", 1); break;
    case 0x05F60DC3:
    case 0x82C91C02: MusicRequest("Sounds/Music/experiment_Elemental_Mystic", 1); break;
    case 0xA34C55ED: MusicRequest("Sounds/Music/music_experiment_02", 1); break;
    case 0xF3278DFC: MusicRequest("Sounds/Music/Music_CastleExterior32kMST", 1); break;
    case 0x9B19ACD1: MusicRequest("Sounds/Music/MusicGraveyard", 1); break;
    case 0xB7B512C0: MusicRequest("Sounds/Music/MusicVillage", 1); break;
    case 0x68BAEC35: MusicRequest("Sounds/Music/challenge_angry_mob", 1); break;
    case 0x92AE284A: MusicRequest("Sounds/Music/challenge_tree_shaking", 1); break;
    case 0x6DBCBC22: MusicRequest("Sounds/Music/challenge_digging", 1); break;
    case 0xDA5AD4E1: MusicRequest("Sounds/Music/challenge_box_breaker", 1); break;
    case 0x1DE8B64E: MusicRequest("Sounds/Music/Music_Village1aMST", 1); break;
    case 0x5CC14310: MusicRequest("Sounds/Music/Music_Graveyard1aMST", 1); break;
    case 0x7B794FF5: MusicRequest("Sounds/Music/challenge_ancient_puzzle", 1); break;
    case 0xA750E8D9: MusicRequest("Sounds/Music/challenge_bug_smasher", 1); break;
    case 0x6D0FCFD5: MusicRequest("Sounds/Music/challenge_mining", 1); break;
    case 0x6E59E316: MusicRequest("Sounds/Music/challenge_alloy_sweeper", 1); break;
    case 0x49D29DC3: MusicRequest("Sounds/Music/challenge_field_repair", 1); break;
    case 0x48012811: MusicRequest("Sounds/Music/challenge_sewer_scooper", 1); break;
    case 0xE300AE91: flag = 0; MusicRequest("Sounds/Music/stinger_success_01", 1); break;
    case 0x4C1E65E6: flag = 0; MusicRequest("Sounds/Music/stinger_fail_01", 1); break;
    case 0x06BE5C73: flag = 1; MusicRequest("Sounds/Music/combat_music_Boss", 1); break;
    case 0x98A15A88: flag = 1; MusicRequest("Sounds/Music/combat_music_Gen", 1); break;
    }
    fn_8020B068(flag);
}

// 0x801EC8C4
// Starts effect `effect` in the first free effect slot (of 8) of record idx in the manager,
// positions its children at the record position and applies the record orientation to them.
void fn_801EC8C4(u8* mgr, s32 idx, u32 effect) {
    if (idx < 0 || idx >= *(s32*)mgr) return;
    s32 k;
    u8* rec = *(u8**)(mgr + 4) + idx * 0xc8;
    for (k = 0; k < 8; k++) {
        if (*(u8**)(rec + 0x88 + k * 4) == NULL) break;
    }
    if (k == 8) {
        // No free slot: drop slot 0
        if (idx >= 0 && idx < *(s32*)mgr) {
            rec = *(u8**)(mgr + 4) + idx * 0xc8;
            u8* old = *(u8**)(rec + 0x88);
            if (old) {
                fn_801C849C(lbl_8047C760, old);
                *(u32*)(*(u8**)(mgr + 4) + idx * 0xc8 + 0x88) = 0;
                *(u32*)(*(u8**)(mgr + 4) + idx * 0xc8 + 0x68) = 0;
                *(u32*)(*(u8**)(mgr + 4) + idx * 0xc8 + 0xa8) = 0;
            } else if (*(u32*)(rec + 0x68) != 0) {
                *(u32*)(rec + 0x68) = 0;
                *(u32*)(*(u8**)(mgr + 4) + idx * 0xc8 + 0xa8) = 0;
            }
        }
        k = 0;
    }
    rec = *(u8**)(mgr + 4) + idx * 0xc8;
    *(u32*)(rec + 0x68 + k * 4) = effect;
    rec = *(u8**)(mgr + 4) + idx * 0xc8;
    *(s32*)(rec + 0xa8 + k * 4) = 1;
    rec = *(u8**)(mgr + 4) + idx * 0xc8;
    f32 pos[4];
    pos[0] = *(f32*)(rec + 0x5c);
    pos[1] = *(f32*)(rec + 0x60);
    pos[2] = *(f32*)(rec + 0x64);
    pos[3] = 0.0f;
    SpawnEffectAt(lbl_8047C760, *(u32*)(rec + 0x68 + k * 4), pos, (u8**)(rec + 0x88 + k * 4));
    rec = *(u8**)(mgr + 4) + idx * 0xc8;
    u8* eff = *(u8**)(rec + 0x88 + k * 4);
    if (eff == NULL) return;
    f32 px = *(f32*)(rec + 0x5c);
    f32 py = *(f32*)(rec + 0x60);
    f32 pz = *(f32*)(rec + 0x64);
    f32 pw = 0.0f;
    for (s32 i = 0; i < *(s32*)(eff + 0xc); i++) {
        u8* child = *(u8**)(eff + 0x14 + i * 4);
        if (child) {
            *(f32*)(child + 0x10) = px;
            *(f32*)(child + 0x14) = py;
            *(f32*)(child + 0x18) = pz;
            *(f32*)(child + 0x1c) = pw;
        }
    }
    rec = *(u8**)(mgr + 4) + idx * 0xc8;
    f32 q[4];
    f32 out[4];
    q[0] = *(f32*)(rec + 0x44);
    q[1] = *(f32*)(rec + 0x48);
    q[2] = *(f32*)(rec + 0x4c);
    q[3] = 0.0f;
    rec = *(u8**)(mgr + 4) + idx * 0xc8;
    fn_801C9DF0(*(u8**)(*(u8**)(rec + 0x88 + k * 4) + 0x14), q, out, 0.0f);
    rec = *(u8**)(mgr + 4) + idx * 0xc8;
    eff = *(u8**)(rec + 0x88 + k * 4);
    f32 a = *(f32*)(rec + 0x44);
    f32 b = *(f32*)(rec + 0x48);
    f32 c = *(f32*)(rec + 0x4c);
    f32 d = 0.0f;
    f32 v[4];
    v[0] = a;
    v[1] = b;
    v[2] = c;
    v[3] = d;
    s32 inRange = 0;
    if (a <= 0.0001f && a >= -0.0001f && b <= 0.0001f && b >= -0.0001f &&
        c <= 0.0001f && c >= -0.0001f && d <= 0.0001f && d >= -0.0001f) {
        inRange = 1;
    }
    if (inRange) {
        // Orientation is (near) zero: use the identity orientation.
        for (s32 i = 0; i < *(s32*)(eff + 0xc); i++) {
            u8* child = *(u8**)(eff + 0x14 + i * 4);
            if (child) {
                f32 id[4];
                id[0] = 0.0f;
                id[1] = 0.0f;
                id[2] = 1.0f;
                id[3] = 0.0f;
                fn_801CA30C(child, id);
            }
        }
    } else {
        for (s32 i = 0; i < *(s32*)(eff + 0xc); i++) {
            u8* child = *(u8**)(eff + 0x14 + i * 4);
            if (child) fn_801CA30C(child, v);
        }
    }
}

// 0x8018A948
// Per-frame update of an animated actor: advances its animation(s), blends the secondary
// clips, steps the attached sub-records, tracks position velocity (+0xC0/+0xCC) and rebases
// the node positions against the root node.
void fn_8018A948(u8* a, f32 dt) {
    static const u32 kHashHead = 0x91661B28u;  // "head"
    f32 pos[4];
    u8* obj = *(u8**)a;
    f32 ratio;
    if (obj != NULL) {
        u8* m = *(u8**)(obj + 0x14);
        u8* an = *(u8**)(m + 0x14);
        if (an != NULL) {
            f32 len = *(f32*)(an + 0x20);
            if (len <= 0.0f) ratio = 1.0f;
            else ratio = *(f32*)(m + 0x24) / len;
        } else {
            ratio = 0.0f;
        }
    } else {
        ratio = 0.0f;
    }
    *(f32*)(a + 0xb8) = ratio;
    if (dt != 0.0f) {
        *(u32*)(a + 0xf0) &= ~0x10u;
        *(u32*)(a + 0xbc) = lbl_805FB854;
    }
    obj = *(u8**)a;
    if (obj == NULL) {
        *(f32*)(a + 0xc0) = 0.0f;
        *(f32*)(a + 0xc4) = 0.0f;
        *(f32*)(a + 0xc8) = 0.0f;
        return;
    }
    if (*(s32*)(*(u8**)(obj + 0x14) + 0x14) == 0) {
        *(f32*)(a + 0xc0) = 0.0f;
        *(f32*)(a + 0xc4) = 0.0f;
        *(f32*)(a + 0xc8) = 0.0f;
        return;
    }
    if (!(*(u32*)(a + 0xf0) & 0x400)) {
        f32 t = dt * *(f32*)(a + 0xb0);
        if (t > 0.0f) {
            u8* p = *(u8**)(a + 0x78);
            if (p != NULL && *(s32*)(p + 0x14) != 0) fn_8016A64C(p, t);
            fn_8016A64C(*(u8**)(*(u8**)a + 0x14), dt * *(f32*)(a + 0xb0));
            obj = *(u8**)a;
            if (*(s32*)(*(u8**)(obj + 0x14) + 0x1c) != 0) {
                *(s32*)(a + 0xd8) = 1;
                u32 fl = *(u32*)(a + 0xf0) | 0x10;
                *(u32*)(a + 0xf0) = fl;
                if (fl & 8) {
                    *(u32*)(a + 0xf0) = fl | 0x400;
                    u8* c = *(u8**)(obj + 0x14);
                    f32 old = *(f32*)(c + 0x24);
                    f32 tm = *(f32*)(*(u8**)(c + 0x14) + 0x20);
                    while (tm < 0.0f) tm += *(f32*)(*(u8**)(c + 0x14) + 0x20);
                    u8* an2 = *(u8**)(c + 0x14);
                    while (tm > *(f32*)(an2 + 0x20)) tm -= *(f32*)(an2 + 0x20);
                    *(f32*)(c + 0x24) = tm;
                    *(s32*)(c + 0x1c) = 0;
                    if (tm < old) fn_8016A3F8(c);
                    fn_8016A43C(c);
                    fn_8016A788(c);
                }
                *(s32*)(*(u8**)(*(u8**)a + 0x14) + 0x1c) = 0;
            }
        }
    }
    f32 w = *(f32*)(a + 0x94);
    if (w > 0.0f) {
        u8* m = *(u8**)(*(u8**)a + 0x14);
        fn_8016AFE8(m, *(u8**)(a + 0x78), m, 1.0f - w);
    }
    fn_8017387C(pos, *(u8**)a);
    w = *(f32*)(a + 0x9c);
    if (w > 0.0f) {
        u8* m = *(u8**)(*(u8**)a + 0x14);
        fn_8016AFE8(m, *(u8**)(a + 0x74), m, 1.0f - w / *(f32*)(a + 0xac));
        *(f32*)(a + 0x9c) = *(f32*)(a + 0x9c) - dt;
    }
    if (*(u8*)(a + 0x8c) != 0) {
        fn_8016A64C(*(u8**)(a + 0x7c), *(f32*)(a + 0x88) * dt);
        if (*(s32*)(*(u8**)(a + 0x7c) + 0x1c) != 0) *(u8*)(a + 0x8c) = 0;
        if (*(u8*)(a + 0x8c) != 0) {
            u8* c = *(u8**)(a + 0x7c);
            f32 left = *(f32*)(*(u8**)(c + 0x14) + 0x20) - *(f32*)(c + 0x24);
            f32 k;
            *(f32*)(a + 0x90) = left;
            k = (left - 0.25f) / -0.25f;
            if (k < 0.0f) {
                k = 0.0f;
            } else if (k > 1.0f) {
                k = 1.0f;
            }
            s32 first = *(s32*)(a + 0x80);
            s32 last = *(s32*)(a + 0x84);
            u8* m = *(u8**)(*(u8**)a + 0x14);
            fn_8016ACB8(m, first, *(u8**)(a + 0x7c), first, m, first, last - first + 1, k);
        }
    }
    fn_80173944(*(u8**)a);
    {
        u8* q = a + 4;
        for (s32 i = 0; i < 3 && *(s32*)(q + i * 0x24) != -1; i++) {
            fn_8018C508(q + i * 0x24, dt);
        }
    }
    if (*(s32*)(a + 0xd8) != 0 && !(*(u32*)(a + 0xf0) & 0x400)) {
        *(s32*)(a + 0xd8) = *(s32*)(a + 0xd8) - 1;
        *(f32*)(a + 0xc0) = *(f32*)(a + 0xcc);
        *(f32*)(a + 0xc4) = *(f32*)(a + 0xd0);
        *(f32*)(a + 0xc8) = *(f32*)(a + 0xd4);
    } else {
        f32 v[3];
        if (dt * *(f32*)(a + 0xb0) > 0.0f) {
            v[0] = pos[0] - *(f32*)(a + 0xdc);
            v[1] = pos[1] - *(f32*)(a + 0xe0);
            v[2] = pos[2] - *(f32*)(a + 0xe4);
        } else {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 0.0f;
        }
        f32 sc = *(f32*)(a + 0xe8);
        f32 x = v[0] * sc;
        f32 y = v[1] * sc;
        f32 z = v[2] * sc;
        *(f32*)(a + 0xc0) = x;
        *(f32*)(a + 0xc4) = y;
        *(f32*)(a + 0xc8) = z;
        *(f32*)(a + 0xcc) = x;
        *(f32*)(a + 0xd0) = y;
        *(f32*)(a + 0xd4) = z;
    }
    *(f32*)(a + 0xdc) = pos[0];
    *(f32*)(a + 0xe0) = pos[1];
    *(f32*)(a + 0xe4) = pos[2];
    u8* m = *(u8**)(*(u8**)a + 0x14);
    u8* root = *(u8**)(*(u8**)a + 0x4);
    s32 n = *(s32*)(m + 0x10);
    u8 flag = lbl_805FB64C;
    for (s32 i = n - 1; i >= 0; i--) {
        u8* node = *(u8**)(*(u8**)a + 0x4) + i * 0x30;
        *(f32*)(node + 0x24) = *(f32*)(node + 0x24) - *(f32*)(root + 0x24);
        *(f32*)(node + 0x28) = *(f32*)(node + 0x28) - *(f32*)(root + 0x28);
        *(f32*)(node + 0x2c) = *(f32*)(node + 0x2c) - *(f32*)(root + 0x2c);
        if (flag != 0) {
            u32 id = *(u32*)(*(u8**)(*(u8**)a + 0xc) + i * 8);
            if (id == kHashHead || id == 0xBC547CD3u || id == 0x9E216B17u) {
                for (s32 o = 0; o <= 0x20; o += 4) {
                    *(f32*)(node + o) = *(f32*)(node + o) * lbl_805F9348;
                }
            }
        }
    }
    *(s32*)(lbl_8047C640 + 0x88) = *(s32*)(lbl_8047C640 + 0x88) + n;
}

// Clears the widget's tint (zero colour at +0x80) and calls its vtable slot 0x2C with 0x20.
static void ScreenSetsClearTint(u8* w) {
    fn_8019BC80(w, 0);
    *(f32*)(w + 0x80) = 0.0f;
    *(f32*)(w + 0x84) = 0.0f;
    *(f32*)(w + 0x88) = 0.0f;
    *(f32*)(w + 0x8c) = 0.0f;
    (*(WidgetVFn*)(*(u8**)(w + 0x138) + 0x2c))(w, 0x20);
}

// 0x8018D70C
// Switches the UI cursor widget of a screen set to element `hash`: hides the old cursor, looks up
// the new one (falling back to the "Cursor" child widget), then shows it and registers the
// cursor controller object (+0x78).
void ScreenSetsActivate(u8* a, u32 hash) {
    u8* w;
    u8* cur = *(u8**)(a + 0x7c);
    if (cur != NULL) {
        w = fn_8019BF64(cur, 0x0517EA02u);
        if (w != NULL) ScreenSetsClearTint(w);
        w = fn_8019BF64(*(u8**)(a + 0x7c), 0x9C1EBBB8u);
        if (w != NULL) ScreenSetsClearTint(w);
        w = fn_8019BF64(*(u8**)(a + 0x7c), 0x382C6918u);
        if (w != NULL) (*(WidgetVFn*)(*(u8**)(w + 0x138) + 0x30))(w, 2);
        if (fn_801B3308() != NULL) fn_801B3378(fn_801B3308(), *(u8**)(a + 0x78));
        cur = *(u8**)(a + 0x7c);
        if (cur != NULL) {
            (*(WidgetVFn*)(*(u8**)(cur + 0x138) + 0x30))(cur, 2);
            if (fn_801B3308() != NULL) fn_801B3378(fn_801B3308(), *(u8**)(a + 0x78));
        } else {
            fn_8018D2B8("UIEngine::ShowCursor : cursor not defined ...\n");
        }
        *(u8**)(a + 0x7c) = NULL;
    }
    *(u8*)(a + 0x80) = 0;
    cur = (u8*)UIFindElement(hash);
    *(u8**)(a + 0x7c) = cur;
    if (cur == NULL) {
        u8* g = (u8*)fn_801953E4(StringHash("Cursor"));
        if (g != NULL) {
            *(u8**)(a + 0x7c) = (u8*)UIWidgetFindChild(g, hash);
            *(u8*)(a + 0x80) = 1;
        }
    }
    u8* sets = (u8*)gScreenSets;
    if (*(u8**)(sets + 0x7c) != NULL) {
        w = fn_8019BF64(*(u8**)(sets + 0x7c), 0x0517EA02u);
        if (w != NULL) ScreenSetsClearTint(w);
        w = fn_8019BF64(*(u8**)(sets + 0x7c), 0x9C1EBBB8u);
        if (w != NULL) ScreenSetsClearTint(w);
        w = fn_8019BF64(*(u8**)(sets + 0x7c), 0x382C6918u);
        if (w != NULL) (*(WidgetVFn*)(*(u8**)(w + 0x138) + 0x30))(w, 2);
    }
    cur = *(u8**)(a + 0x7c);
    if (cur != NULL) {
        (*(WidgetVFn*)(*(u8**)(cur + 0x138) + 0x2c))(cur, 2);
        if (fn_801B3308() != NULL) {
            if (*(u8**)(a + 0x78) == NULL) {
                u8* o = (u8*)fn_801D40E0(0xbc);
                if (o != NULL) o = fn_801F61AC(o, 0);
                *(u8**)(a + 0x78) = o;
                *(f32*)(o + 0xb0) = 1.0f;
                *(f32*)(o + 0xb4) = 1.25f;
            }
            fn_801B3310(fn_801B3308(), *(u8**)(a + 0x78));
        }
    } else {
        fn_8018D2B8("UIEngine::ShowCursor : cursor not defined ...\n");
    }
}

// 0x801B55F4
// Writes the object's 3x4 world transform to out. With a parent index (!= -1) and a valid parent
// entry, the object's matrix is concatenated with the parent matrix; otherwise copied as is.
void fn_801B55F4(u8* obj, f32* out, s32 idx) {
    if ((*(ObjGetFn*)(*(u8**)obj + 0x10))(obj) == NULL) return;
    f32* a = (f32*)fn_801DA798((*(ObjGetFn*)(*(u8**)obj + 0xc))(obj));
    if (idx != -1) {
        f32* b = (f32*)fn_8018C39C((u8*)(*(ObjGetFn*)(*(u8**)obj + 0x10))(obj), idx);
        if (b != NULL) {
            f32 o0 = (a[6] * b[2]) + ((a[0] * b[0]) + (a[3] * b[1]));
            f32 o1 = (a[7] * b[2]) + ((a[1] * b[0]) + (a[4] * b[1]));
            f32 o2 = (a[8] * b[2]) + ((a[2] * b[0]) + (a[5] * b[1]));
            f32 o3 = (a[6] * b[5]) + ((a[0] * b[3]) + (a[3] * b[4]));
            f32 o4 = (a[7] * b[5]) + ((a[1] * b[3]) + (a[4] * b[4]));
            f32 o5 = (a[8] * b[5]) + ((a[2] * b[3]) + (a[5] * b[4]));
            f32 o6 = (a[6] * b[8]) + ((a[0] * b[6]) + (a[3] * b[7]));
            f32 o7 = (a[7] * b[8]) + ((a[1] * b[6]) + (a[4] * b[7]));
            f32 o8 = (a[8] * b[8]) + ((a[2] * b[6]) + (a[5] * b[7]));
            f32 o9 = a[9] + ((a[6] * b[11]) + ((a[0] * b[9]) + (a[3] * b[10])));
            f32 o10 = a[10] + ((a[7] * b[11]) + ((a[1] * b[9]) + (a[4] * b[10])));
            f32 o11 = a[11] + ((a[8] * b[11]) + ((a[2] * b[9]) + (a[5] * b[10])));
            out[0] = o0;
            out[1] = o1;
            out[2] = o2;
            out[3] = o3;
            out[4] = o4;
            out[5] = o5;
            out[6] = o6;
            out[7] = o7;
            out[8] = o8;
            out[9] = o9;
            out[10] = o10;
            out[11] = o11;
            return;
        }
    }
    for (s32 i = 0; i < 12; i++) out[i] = a[i];
}

// Writes one BP register load (0x61 command + 32-bit value) to the write-gather pipe.
static inline void BPWrite(u32 v) {
    GXWGFifo._u8 = 0x61;
    GXWGFifo._u32 = v;
}

// Closes the current display list, submits it to the render thread and starts the next one.
static void RendCycleDL(void) {
    RendDLEnd((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94));
    s32 left = 0x24000 - lbl_805FB4C8;
    if (left != 0) {
        fn_80142ED0(RendDLGetBase((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94)) + lbl_805FB4C8, left);
    }
    if (lbl_805FB4D4 != 0) {
        lbl_805FB4DC += 1;
    } else {
        RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + lbl_805FB4BC * 0x14), lbl_805FB4E0 == 0);
    }
    lbl_805FB4C0 = 0;
    lbl_805FB4D0 = NULL;
    lbl_805FB4C4 = 0;
    lbl_805FB4C8 = 0;
    lbl_805FB4CC = 0;
    if (lbl_805FB4E0 != 0) lbl_805FB4E8 += 1;
    s32 next = lbl_805FB4BC + 1;
    lbl_805FB4BC = next;
    if (next >= 0x11) lbl_805FB4BC = 0;
}

// 0x801508AC
// End of frame: emits the PE finish/token BP writes, flushes the last display list, then starts
// a fresh display list that holds the XFB copy for this frame buffer and hands both off.
void RendEndFrame(void) {
    s32 parity = lbl_805FB518 & 1;
    if (lbl_805F9231 == 0) {
        lbl_805F9231 = 1;
        BPWrite(0xFE000011u);
        BPWrite((u32)(lbl_805F9232 << 4) | 0x40000001u);
    }
    lbl_805F9232 = 1;
    if (lbl_805F9231 != 0) {
        BPWrite(0xFE000011u);
        BPWrite(0x40000011u);
    }
    BPWrite(0xFE000018u);
    BPWrite(0x41000018u);
    BPWrite(0xFE00000Eu);
    BPWrite(0x40000006u);
    RendCycleDL();
    fn_8015E654(0, (u32)parity, 0, 0, 0);  // r3 = lbl_805FB594 (unused by callee)
    if (lbl_805FB4D4 != 0 && RendHasFreeDL() == 0 && lbl_805FB4D4 != 0) {
        RendThreadSync();
        s32 head = lbl_805FB4D8;
        s32 pending = lbl_805FB4DC;
        while (pending > 0) {
            RendThreadSubmitDL((RendDLRecord*)(sRendDLRecords + head * 0x14), lbl_805FB4E0 == 0);
            head = lbl_805FB4D8 + 1;
            lbl_805FB4D8 = head;
            if (head >= 0x11) {
                lbl_805FB4D8 = 0;
                head = 0;
            }
            pending = lbl_805FB4DC - 1;
            lbl_805FB4DC = pending;
        }
        lbl_805FB4D4 = 0;
        lbl_805FB4D8 = 0;
        lbl_805FB4DC = 0;
    }
    RendWaitFreeDL();
    RendDLBegin((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94), (RendDLRecord*)(sRendDLRecords + lbl_805FB4BC * 0x14));
    lbl_805FB4C0 = 1;
    lbl_805FB4C4 = 0x24000;
    lbl_805FB4C8 = 0x24000;
    lbl_805FB4CC = 0;
    lbl_805FB4D0 = RendDLGetBase((RendDL*)(sRendDLs + lbl_805FB4BC * 0x94));
    lbl_805FB4EC -= 1;
    void* fb = VideoGetFrameBuffer(&gVideo, parity);
    s32 w = VideoGetWidth(&gVideo);
    s32 h = VideoGetHeight(&gVideo);
    BPWrite(((u32)(lbl_805FB504 << 2) | 0x41000000u) | 0x4B8u);
    BPWrite(0x4000001Fu);
    BPWrite((u32)((lbl_805FB500 | 0x40) & 0x1F) | 0x43000000u);
    u8* t = lbl_803765A0 + lbl_805F9218 * 7;
    BPWrite((u32)t[0] | ((u32)t[1] << 6) | ((u32)t[2] << 12) | ((u32)t[3] << 18) | 0x53000000u);
    BPWrite((u32)t[4] | ((u32)t[5] << 6) | ((u32)t[6] << 12) | 0x54000000u);
    BPWrite(0x49000000u);
    BPWrite((u32)((h - 1) << 10) | ((u32)(w - 1) | 0x4A000000u));
    BPWrite((u32)(((uintptr_t)fb >> 5) & 0xFFFFFF) | 0x4B000000u);
    BPWrite((u32)(w >> 4) | 0x4D000000u);
    BPWrite(0x52004003u);
    RendCycleDL();
    fn_8015E6D0(NULL, parity, 0, 0, 0);  // asm value arg is stale r7
    fn_8015E74C(NULL, lbl_805FB518, 0, 0, 0);  // asm value arg is stale r7
    lbl_805FB51C = 0;
    lbl_805FB518 += 1;
}

// Number of valid friend records (counted under the friend-list lock).
static s32 FriendListCount(void) {
    fn_8023A5B0(lbl_80556AB0);
    lbl_805FBB48 += 1;
    s32 n = fn_802E64A0(lbl_80556E80 + 0x40, 0x40);
    fn_8023A68C(lbl_80556AB0);
    lbl_805FBB48 -= 1;
    return n;
}

// 0x8013B53C
// Rebuilds the friend display order (all used records), sorts it by name (mode 0) or by key
// (mode 1) with a shrinking-gap bubble sort, then updates the sort toggle buttons and marks the
// four visible entries.
void FriendListSort(BOOL a0) {
    s32 i;
    s32 n = 0;
    s32 count;
    s32 gap;
    s32 swapped;
    lbl_805F9028 = a0;
    for (i = 0; i < 0x40; i++) {
        lbl_803F0E30[i] = -1;
        if ((s8)lbl_80556E80[i * 0x20 + 0x340] != 0) lbl_803F0E30[n++] = i;
    }
    swapped = 1;
    gap = FriendListCount();
    if (a0 == 0) {
        while (swapped != 0 || gap > 1) {
            s32* lo = lbl_803F0E30;
            gap = (gap + 1) / 2;
            s32* hi = lbl_803F0E30 + gap;
            swapped = 0;
            for (i = 0; i < FriendListCount() - gap; i++, lo++, hi++) {
                s32 x = *lo;
                s32 y = *hi;
                if (strcmp((const char*)(lbl_80556E80 + y * 0x20 + 0x340),
                           (const char*)(lbl_80556E80 + x * 0x20 + 0x340)) < 0) {
                    *hi = x;
                    swapped = 1;
                    *lo = y;
                }
            }
        }
    } else if (a0 == 1) {
        while (swapped != 0 || gap > 1) {
            s32* lo = lbl_803F0E30;
            gap = (gap + 1) / 2;
            s32* hi = lbl_803F0E30 + gap;
            swapped = 0;
            for (i = 0; i < FriendListCount() - gap; i++, lo++, hi++) {
                s32 y = *hi;
                s32 x = *lo;
                if (lbl_803F0F30[y] < lbl_803F0F30[x]) {
                    *hi = x;
                    swapped = 1;
                    *lo = y;
                }
            }
        }
    }
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        u8* w = (u8*)ScreenSetFindWidget(set, 0x9DE0B3FEu);
        if (w != NULL && UIWidgetFindChild(w, 0xAF03B431u) != NULL) {
            u8* byName = (u8*)UIWidgetFindChild(w, 0x365A0BEFu);
            u8* byKey = (u8*)UIWidgetFindChild(w, 0xC2F01FBFu);
            if (byName != NULL && byKey != NULL) {
                if (a0 == 0) {
                    (*(WidgetVFn*)(*(u8**)(byName + 0x138) + 0x2c))(byName, 2);
                    (*(WidgetVFn*)(*(u8**)(byKey + 0x138) + 0x30))(byKey, 2);
                } else {
                    (*(WidgetVFn*)(*(u8**)(byName + 0x138) + 0x30))(byName, 2);
                    (*(WidgetVFn*)(*(u8**)(byKey + 0x138) + 0x2c))(byKey, 2);
                }
            }
        }
    }
    FriendListHideFloaty(1);
    lbl_803F0F30[lbl_803F0E30[lbl_805FB35C]] |= 1;
    lbl_803F0F30[lbl_803F0E30[lbl_805FB35C + 1]] |= 1;
    lbl_803F0F30[lbl_803F0E30[lbl_805FB35C + 2]] |= 1;
    lbl_803F0F30[lbl_803F0E30[lbl_805FB35C + 3]] |= 1;
    fn_8013AFF0();
}

typedef s32 (*LinkQueryFn)(void* obj);
typedef void (*MsgFn)(void* data, u8* entry);

// Steps the packet ring's tail back by one slot (or resets it when empty).
static void PacketRingStepBack(void) {
    s32 tail = *(s32*)(lbl_804DD568 + 0xc);
    if (tail == *(s32*)(lbl_804DD568 + 0x8)) {
        *(s32*)(lbl_804DD568 + 0xc) = -1;
        *(s32*)(lbl_804DD568 + 0x8) = -1;
    } else {
        s32 v = tail - 1;
        if (v < 0) v = *(s32*)(lbl_804DD568 + 0x10) - 1;
        *(s32*)(lbl_804DD568 + 0xc) = v;
    }
}

// 0x801F3BB8
// Processes the current network packet under the packet lock: drops it if the link is down,
// otherwise feeds pending queue data into it, completes a pending message, and either retries
// (type 0x18 packets, up to 30 times) or finalises it with a checksum.
void fn_801F3BB8(void) {
    if (lbl_805FB95C == NULL) return;
    if (lbl_805FB9D8 == 0) fn_801F5E4C(lbl_804FB5E0, 2);
    lbl_805FB9D8 += 1;
    if (lbl_805FB970 == 0 || (*(LinkQueryFn*)(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) == 0 ||
        lbl_805FB974 == 0) {
        PacketRingStepBack();
        lbl_805FB95C = NULL;
        lbl_805FB9D8 -= 1;
        if (lbl_805FB9D8 == 0) fn_801F5E48(lbl_804FB5E0, 2);
        return;
    }
    if (*(s32*)(lbl_804FB5A8 + 0) != 0 && *(s32*)(lbl_804FB5A8 + 4) != 0 && lbl_805FB994 == 0) {
        s32 limit = lbl_80386064[0x10];
        if (fn_801F5B94(lbl_805FB95C) > (u32)limit) {
            u8* ev = (u8*)EventPost(0x6f, NULL);
            s32 n = (s32)fn_801F5B94(lbl_805FB95C);
            u8* dst = fn_801F5BA0(lbl_805FB95C);
            s32 cnt;
            if (*(s32*)(lbl_804FB5A8 + 0) == 0) {
                cnt = 0;
            } else {
                u8* blk = *(u8**)(lbl_804FB5A8 + 0xc);
                s32 pos = *(s32*)(lbl_804FB5A8 + 0x8);
                cnt = *(s32*)(blk + 4) - pos;
                if (n < cnt) cnt = n;
                memcpy(dst, *(u8**)(blk + 8) + pos, (size_t)cnt);
                *(s32*)(lbl_804FB5A8 + 0x8) = pos + cnt;
                if ((u32)(pos + cnt) == *(u32*)(*(u8**)(lbl_804FB5A8 + 0xc) + 4)) {
                    *(s32*)(lbl_804FB5A8 + 0x4) = 0;
                }
            }
            *(s32*)(ev + 4) = cnt;
        }
    }
    {
        s32 need = lbl_80386064[0xf];
        if (fn_801F5B94(lbl_805FB95C) >= (u32)need) {
            u8* msg = lbl_804FB5C4;
            if (*(s32*)(msg + 0) != 0 && *(s32*)(msg + 0x10) != 0) {
                if (EventPost(0x6e, NULL) != NULL) {
                    s32 id = *(s32*)(msg + 0x18);
                    if (id != -1) {
                        u8* e = gScriptFuncs + id * 0xc;
                        (*(MsgFn*)(e + 4))(*(void**)(msg + 0xc), e);
                    }
                    BufferFree(*(void**)(*(u8**)(lbl_804FB5C4 + 0xc) + 8));
                    BufferFree(*(void**)(lbl_804FB5C4 + 0xc));
                    *(void**)(lbl_804FB5C4 + 0xc) = NULL;
                    *(s32*)(lbl_804FB5C4 + 0) = 0;
                    *(s32*)(lbl_804FB5C4 + 0x10) = 0;
                }
            }
        }
    }
    if (lbl_805FB994 != 0) {
        s32 up = 0;
        if (lbl_805FB970 != 0 && lbl_805FB974 != 0 && lbl_805FB964 != NULL &&
            (*(LinkQueryFn*)(*(u8**)lbl_805FB964 + 0x34))(lbl_805FB964) != 0) {
            up = 1;
        }
        if (up == 0) lbl_805FB994 = 0;
    }
    if (lbl_805FB994 != 0 && fn_801F5B94(lbl_805FB95C) > 0x64) {
        lbl_805FB9D8 -= 1;
        if (lbl_805FB9D8 == 0) fn_801F5E48(lbl_804FB5E0, 2);
        return;
    }
    u8* pkt = lbl_805FB95C;
    if (*(u32*)(pkt + 0x10) == 0x18 && lbl_805FB96C < 0x1e) {
        PacketRingStepBack();
        lbl_805FB95C = NULL;
        lbl_805FB96C += 1;
        lbl_805FB9D8 -= 1;
        if (lbl_805FB9D8 == 0) fn_801F5E48(lbl_804FB5E0, 2);
    } else {
        lbl_805FB96C = 0;
        lbl_805F97F8 += 1;
        *(f32*)(pkt + 0x8) = -1.0f;
        *(f32*)(lbl_805FB95C + 0xc) = lbl_805FB99C;
        if (fn_801F5A90(lbl_805FB95C) == 0) fn_801F5A90(lbl_805FB95C);
        *(u32*)lbl_805FB95C = fn_80143974(0, lbl_805FB95C + 0x18, *(s32*)(lbl_805FB95C + 0x10) - 0x18);
        lbl_805FB95C = NULL;
        lbl_805FB9D8 -= 1;
        if (lbl_805FB9D8 == 0) fn_801F5E48(lbl_804FB5E0, 2);
    }
}

}  // extern "C"
