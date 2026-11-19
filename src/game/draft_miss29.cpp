// Monster Lab: menu name entry, matchup UI, score table, monster name copy, mystic brew hide
// helpers, music play and multiplayer cleanup. Small functions verified against the asm.
#include "types.h"
#include "game/part.h"
extern "C" {
#include "game/script_callbacks.h"
}

extern "C++" void* gScreenSets;  // 0x805FB654

extern "C" {

// Prototypes of callees (defined elsewhere).
void* CharacterFind(s32 id);                                   // 0x800E876C
void* EventPost(s32 id, void* sender);                         // 0x801F3918
void* MatchupGetRemoteData(void);                              // 0x80113974
void PartCopy(const PartInstance* src, PartInstance* dst);     // 0x8021EE9C
void* ScreenSetFindWidget(void* set, u32 hash);                // 0x80198E34
void* ScreenSetsCurrent(void* sets);                           // 0x8018D1E4
u32 StringHash(const char* s);                                 // 0x801EB704
void UISetText(void* el, const char* text, s32 a);             // 0x8019EFC8
void* UIWidgetFindChild(void* widget, u32 hash);               // 0x80199AEC
void UIWidgetPlayAnim(void* w, u32 anim);                      // 0x80199B38
void* WorldFindLocator(void* world, u32 hash);                 // 0x8012AA50
BOOL ZoneIsLoaded(void* world, s32 zone);                      // 0x8012F408
s32 fn_800BE3A8(u8* proj, f32* a, f32* b, s32 code, f32 f1, f32 f2);  // 0x800BE3A8
s32 fn_800BF18C(s32 p);                                        // 0x800BF18C
void fn_8010A374(void);                                        // 0x8010A374
s32 fn_801A3010(void* obj, void* data);                        // 0x801A3010
s32 fn_801B6F34(void* obj);                                    // 0x801B6F34
s32 fn_801E9678(void* list, void* handle);                     // 0x801E9678
void* fn_801E9D18(void* p, u32 hash, s32 size);                // 0x801E9D18
s32 fn_8020AB5C(void);                                         // 0x8020AB5C
void fn_802209D0(void (*cb)(void));                            // 0x802209D0
void fn_80220A1C(s32 a);                                       // 0x80220A1C
void fn_8023A68C(u8* p);                                       // 0x8023A68C
void fn_802E4970(void);                                        // 0x802E4970
char* strncpy(char* dst, const char* src, size_t n);           // 0x80338854

// Globals.
extern u8 gWorldState[];                 // 0x803E6D00
extern char lbl_803F2230[];              // 0x803F2230 (name entry buffer)
extern u8 lbl_803C5DB0[];                // 0x803C5DB0 (score trigger table, 0x8C bytes per entry, count at +0xAF0)
extern u8 lbl_80556AB0[];                // 0x80556AB0
extern u8 lbl_80556AC8[];                // 0x80556AC8
extern char lbl_80559CF8[];              // 0x80559CF8 (music name buffer, 0x80 bytes)
extern u8 lbl_805F8280;                  // 0x805F8280 (character appended by NameAddSpace)
extern u8 lbl_805F9080;                  // 0x805F9080
extern f32 lbl_805F9970;                 // 0x805F9970
extern f32 lbl_805F9974;                 // 0x805F9974
extern f32 lbl_805F9978;                 // 0x805F9978
extern f32 lbl_805F997C;                 // 0x805F997C
extern f32 lbl_806004F0;                 // 0x806004F0 (zero constant used by MusicPlay)
extern u8* lbl_805FAF78;                 // 0x805FAF78 (monster / name record pointer)
extern u8* lbl_805FAFE8;                 // 0x805FAFE8 (monster record pointer)
extern u8 lbl_805FB3B2;                  // 0x805FB3B2
extern u8 lbl_805FB3B3;                  // 0x805FB3B3
extern s32 lbl_805FB3B8;                 // 0x805FB3B8
extern u8* lbl_805FB3BC;                 // 0x805FB3BC
extern u8* lbl_805FB3C0;                 // 0x805FB3C0
extern f32 lbl_805FB3D8;                 // 0x805FB3D8
extern s32 lbl_805FB3E8;                 // 0x805FB3E8
extern s32 lbl_805FB408;                 // 0x805FB408 (name entry length)
extern void* lbl_805FB964;               // 0x805FB964 (movie player)
extern s32 lbl_805FBB48;                 // 0x805FBB48
extern s32 lbl_805FBB6C;                 // 0x805FBB6C
extern u8 lbl_805FBBB3;                  // 0x805FBBB3 (music enabled flag)
extern f32 lbl_805FE280;                 // 0x805FE280
extern f32 lbl_805FE284;                 // 0x805FE284
extern f32 lbl_805FE2C8;                 // 0x805FE2C8
extern f32 lbl_805FE2F0;                 // 0x805FE2F0
extern f32 lbl_805FE308;                 // 0x805FE308
extern f32 lbl_805FE330;                 // 0x805FE330
extern f32 lbl_805FEFB4;                 // 0x805FEFB4

typedef s32 (*MonsterVFn)(u8* obj, s32 arg);
static const u32 kHashName = 0x68B693B2u;  // "NAME"

// Refreshes the name text element on the current screen set from the name buffer.
static void NameRefreshText(void) {
    u8* list = *(u8**)((u8*)gScreenSets + 0x88);
    if (list != NULL) {
        u8* w = (u8*)ScreenSetFindWidget(list, kHashName);
        if (w != NULL) {
            u8* el = (u8*)UIWidgetFindChild(w, kHashName);
            if (el != NULL) UISetText(el, lbl_803F2230, 1);
        }
    }
}

// Appends one character to the name buffer (max 15 letters; past that it overwrites the last slot).
static void NameAppendChar(u8 c) {
    u8* name = lbl_805FAF78;
    if (name == NULL) return;
    s32 count = *(s32*)(name + 0xBF8);
    if (count == 0 && c == 0x20) return;
    if (c == 0) return;
    if (count < 0xF) {
        *(u8*)(name + 0xAE8 + count) = c;
        count += 1;
        *(s32*)(name + 0xBF8) = count;
        *(u8*)(name + 0xAE8 + count) = 0;
    } else {
        *(u8*)(name + 0xAE7 + count) = c;
    }
}

// Sets the three hide-position floats in the flag word, then the object is hidden.
static void MysticBrewHideFlags(u8* obj) {
    *(u32*)(obj + 0x20) |= 0x8000;
    *(u32*)(obj + 0x20) |= 0x10000000;
    *(u32*)(obj + 0x20) |= 0x4000;
    *(u32*)(obj + 0x20) |= 0x10000000;
}

// 0x8013EF74
// Adds a space to the multiplayer name entry (up to 15 chars) and refreshes the name text.
void MPNameAddSpace(void) {
    s32 n = lbl_805FB408;
    if (n != 0 && (u32)n < 0xF) {
        lbl_803F2230[n] = 0x20;
        lbl_805FB408 = n + 1;
        lbl_803F2230[n + 1] = 0;
        NameRefreshText();
    }
}

// 0x8013ED6C
// Deletes the last letter of the multiplayer name entry and refreshes the name text.
void MPNameDeleteLetter(void) {
    if (lbl_805FB408 != 0) {
        lbl_805FB408 -= 1;
        lbl_803F2230[lbl_805FB408] = 0;
        NameRefreshText();
    }
}

// 0x8013DD50
// Accept button for a matchup: posts event 0x41 and sets the accepted flag.
void MatchupAcceptFight(void) {
    EventPost(0x41, NULL);
    lbl_805FB3B8 = 1;
}

// 0x8013DD80
// Cancel button for a matchup: posts event 0x42 and clears the flags.
void MatchupCancel(void) {
    EventPost(0x42, NULL);
    lbl_805FB3B8 = 0;
    lbl_805FB3B3 = 0;
}

// 0x8013DDB4
// Per-frame matchup update: fetches the remote monster once the local creature is loaded,
// counts down the wait timer and then plays the follow-up virtual call on the monster object.
void MatchupShowMyMonster(void) {
    if (lbl_805FB3BC != NULL) {
        if (Creatures_GetCreatureLoaded() != 0 && !(*(u32*)(lbl_805FB3BC + 0x64) & 2) && lbl_805FB3E8 == 0) {
            u8* remote = (u8*)MatchupGetRemoteData();
            lbl_805FB3C0 = remote;
            if (remote != NULL) {
                fn_801A3010(lbl_805FB3BC, remote);
                lbl_805FB3E8 = 3;
            }
        }
        if (lbl_805FB3E8 > 0) {
            lbl_805FB3E8 -= 1;
            if (lbl_805FB3E8 == 0) {
                if (lbl_805F9080 != 0) {
                    lbl_805F9080 = 0;
                    UIElement_SetVisibleFlag(0x363BBFFF, 0);
                }
                (*(MonsterVFn*)(*(u8**)(lbl_805FB3BC + 0x138) + 0x2C))(lbl_805FB3BC, 2);
            }
        }
    }
}

// 0x8013DC54
// Switches the matchup status widget animation (arg 1 = fighting, otherwise idle) when the state changes.
void MatchupShowStatus(u32 arg0) {
    if (lbl_805FB3B2 == arg0) return;
    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    void* w = ScreenSetFindWidget(set, 0x9631294Eu);
    if (w == NULL || UIWidgetFindChild(w, 0x4DD41D3Du) == NULL || UIWidgetFindChild(w, 0xDE4906B0u) == NULL) return;
    if (arg0 == 1) {
        UIWidgetPlayAnim(w, 0xBFD893EFu);
        lbl_805FB3D8 = lbl_805FEFB4;
    } else {
        UIWidgetPlayAnim(w, 0x77C91068u);
    }
    lbl_805FB3B2 = (u8)arg0;
}

// 0x801F28C8
// Stops the bouncing alpha on an overlay mesh: looks up the mesh entry by its name in the zone list.
void MeshStopBouncingOverlayAlpha(void* a0, void* a1) {
    if (a1 == NULL) return;
    u8 zone = *(u8*)((u8*)a0 + 0x1);
    if (ZoneIsLoaded(gWorldState, (s32)zone) == 0) return;
    u8* world = *(u8**)gWorldState;
    u8* list = *(u8**)(world + 0x1C8) + zone * 0xC;
    u8* rec = *(u8**)((u8*)a1 + 0x10);
    const char* name = (rec != NULL) ? *(const char**)(rec + 0x5C) : NULL;
    if (name == NULL) return;
    u8* entry = (u8*)fn_801E9D18(list, StringHash(name), 0xF8);
    if (entry == NULL) return;
    u16 handle = *(u16*)(entry + 0x7C);
    fn_801E9678(list, &handle);
}

// 0x80075590
// Adds a delta to a score entry (found by id in the trigger table), clamps it to [min, max]
// and stores it. The sub-entry loop in the asm has empty bodies, so it has no effect.
// Note: a missing id returns early here; the asm would dereference NULL.
void ModifyScore(u32 a0, f32 a1) {
    u8* entry = NULL;
    u32 count = *(u32*)(lbl_803C5DB0 + 0xAF0);
    for (u32 i = 0; i < count; i++) {
        u8* e = lbl_803C5DB0 + i * 0x8C;
        if (a0 == *(u32*)(e + 0x4)) {
            entry = e;
            break;
        }
    }
    if (entry == NULL) return;
    f32 v = *(f32*)(entry + 0x0) + a1;
    f32 lo = *(f32*)(entry + 0xC);
    if (v < lo) v = lo;
    f32 hi = *(f32*)(entry + 0x8);
    if (v > hi) v = hi;
    *(f32*)(entry + 0x0) = v;
}

// 0x8020EBB8
// Copies a monster's name and its five parts from src into dst (part slots at dst+4).
void MonsterCopyName(u8* dst, u8* src) {
    *(u16*)(dst + 0x20) = *(u16*)(src + 0x20);
    u32 i = 0;
    for (; i < *(u16*)(dst + 0x20); i++) {
        *(u16*)(dst + 0x22 + i * 2) = *(u16*)(src + 0x22 + i * 2);
    }
    *(u16*)(dst + 0x22 + i * 2) = 0;
    *(s32*)(dst + 0x18) = *(s32*)(src + 0x18);
    *(s32*)(dst + 0x1C) = *(s32*)(src + 0x1C);
    *(u8*)(dst + 0x48) = *(u8*)(src + 0x48);
    *(u8*)(dst + 0x49) = *(u8*)(src + 0x49);
    *(u8*)(dst + 0x4A) = *(u8*)(src + 0x4A);
    *(s32*)(dst + 0x44) = *(s32*)(src + 0x44);
    for (u32 p = 0; p < 5; p++) {
        PartInstance* sp = (PartInstance*)(src + 0x4C + p * 0x30);
        PartInstance* dp = (PartInstance*)(dst + 0x4C + p * 0x30);
        PartCopy(sp, dp);
        ((void**)(dst + 4))[p] = (*(s32*)dp != -1) ? (void*)dp : NULL;
    }
}

// 0x800320F8
// Returns 1 when the value (field 0xD0 / field 0x9C of the record at lbl_805FAFE8) halved is non-zero.
s32 MonsterFeelSuccess(void) {
    u8* m = lbl_805FAFE8;
    s32 q = *(s32*)(m + 0xD0) / *(s32*)(m + 0x9C);
    s32 h = q / 2;
    return h != 0;
}

// 0x800F8290
// Moves an entity to a locator's position and sets its flag word bits.
void MoveEntityToLocator(u32 a0, u32 a1) {
    u8* ent = (u8*)CharacterFind((s32)a0);
    u8* loc = (u8*)WorldFindLocator(gWorldState, a1);
    if (ent == NULL || loc == NULL) return;
    u8* obj = *(u8**)(ent + 0x4);
    *(f32*)(obj + 0x0) = *(f32*)(loc + 0x8);
    *(f32*)(obj + 0x4) = *(f32*)(loc + 0xC);
    *(f32*)(obj + 0x8) = *(f32*)(loc + 0x10);
    *(u32*)(obj + 0x20) |= 0x8000;
    *(u32*)(obj + 0x20) |= 0x10000000;
    *(u32*)(obj + 0x20) |= 0x4000;
    *(u32*)(obj + 0x20) |= 0x10000000;
}

// 0x8013BD7C
// Tears down multiplayer state: resets the sub-systems and signals the game flow.
void MultiplayerCleanup(void) {
    fn_8010A374();
    fn_802209D0(NULL);
    fn_80220A1C(0);
    MatchupCleanup();
    if (lbl_805FB964 != NULL && *(u8*)((u8*)lbl_805FB964 + 0xB0) == 0) {
        fn_802E4970();
    }
    lbl_805FBB6C = 0;
    fn_8023A68C(lbl_80556AB0);
    lbl_805FBB48 -= 1;
    fn_8023A68C(lbl_80556AC8);
    GameFlowSignal((s32)0xCF1260FE, 0, 0);
}

// 0x8020AA94
// Queues a music track name. When music is enabled and the current fade value is non-negative
// the name is copied; otherwise the fade timers are set first. Returns 1 when the name was taken.
s32 MusicPlay(const char* name, u8 loop, f32 volume) {
    (void)loop;  // not read by the asm
    if (name == NULL) return 0;
    if (lbl_805FBBB3 == 0) return fn_8020AB5C();
    if (lbl_805F9974 >= lbl_806004F0) {
        strncpy(lbl_80559CF8, name, 0x7F);
        lbl_80559CF8[0x7F] = 0;
    } else {
        if (volume <= lbl_806004F0) return 0;
        lbl_805F9970 = volume;
        lbl_805F9974 = volume;
        strncpy(lbl_80559CF8, name, 0x7F);
        lbl_80559CF8[0x7F] = 0;
        lbl_805F997C = lbl_805F9978;
    }
    return 1;
}

// 0x800BD558
// Fires the ghost's projectile: builds two position vectors (object and target) and calls the
// projectile spawner with a kind code picked from the projectile type at +0x264.
void MysticBrewGhost_FireProjectile(void* a0) {
    u8* p = (u8*)a0;
    if (p == NULL || *(s32*)(p + 0x29C) == 0) return;
    s32 kind = *(s32*)(p + 0x264);
    s32 code = (s32)0xE1571BAAu;
    if (kind == 3) {
        code = (s32)0x194DFC87u;
    } else if (kind == 2) {
        code = (s32)0x7F751A38u;
    }
    u8* obj = *(u8**)(p + 0x4);
    u8* tgt = *(u8**)(*(u8**)(p + 0x2A0) + 0x4);
    f32 va[3];
    f32 vb[3];
    va[0] = *(f32*)(obj + 0x0);
    va[1] = *(f32*)(obj + 0x4);
    va[2] = *(f32*)(obj + 0x8);
    vb[0] = *(f32*)(tgt + 0x0);
    vb[1] = *(f32*)(tgt + 0x4);
    vb[2] = *(f32*)(tgt + 0x8);
    fn_800BE3A8(*(u8**)(p + 0x29C), va, vb, code, *(f32*)(p + 0x274), *(f32*)(p + 0x278));
    *(s32*)(p + 0x29C) = 0;
}

// 0x800BD490
// Hides the ghost: clears its state, puts the object at a fixed hidden position and frees its
// attached handle, then runs the common hide routine.
void MysticBrewGhost_PositionToHide(void* a0) {
    u8* p = (u8*)a0;
    if (p == NULL) return;
    *(u8*)(p + 0x298) = 0;
    u8* obj = *(u8**)(p + 0x4);
    *(f32*)(obj + 0x0) = lbl_805FE280;
    *(f32*)(obj + 0x4) = lbl_805FE284;
    *(f32*)(obj + 0x8) = lbl_805FE280;
    MysticBrewHideFlags(obj);
    s32 h = *(s32*)(p + 0x29C);
    *(s32*)(p + 0x2A0) = 0;
    if (h != 0) {
        fn_800BF18C(h);
        *(s32*)(p + 0x29C) = 0;
    }
    fn_801B6F34(a0);
}

// 0x800BE000
// Hides the power-up: resets its kind, moves the object to a fixed hidden position, then runs
// the common hide routine.
void MysticBrewPowerUp_PositionToHide(void* a0) {
    u8* p = (u8*)a0;
    if (p == NULL) return;
    *(u8*)(p + 0x264) = 0;
    u8* obj = *(u8**)(p + 0x4);
    *(f32*)(obj + 0x0) = lbl_805FE2C8;
    *(f32*)(obj + 0x4) = lbl_805FE2F0;
    *(f32*)(obj + 0x8) = lbl_805FE2C8;
    MysticBrewHideFlags(obj);
    fn_801B6F34(a0);
}

// 0x800BF218
// Hides the projectile: marks it as hidden, moves the object to a fixed hidden position, then
// runs the common hide routine.
void MysticBrewProjectile_PositionToHide(void* a0) {
    u8* p = (u8*)a0;
    if (p == NULL) return;
    *(s32*)(p + 0x290) = 1;
    u8* obj = *(u8**)(p + 0x4);
    *(f32*)(obj + 0x0) = lbl_805FE308;
    *(f32*)(obj + 0x4) = lbl_805FE308;
    *(f32*)(obj + 0x8) = lbl_805FE330;
    MysticBrewHideFlags(obj);
    fn_801B6F34(a0);
}

// 0x8001D2D0
// Adds the character at a0 to the name entry (NameAppendChar rules).
void NameAddLetter(const char* a0) {
    NameAppendChar(*(u8*)a0);
}

// 0x8001D344
// Adds the space character stored at lbl_805F8280 to the name entry.
void NameAddSpace(void) {
    NameAppendChar(lbl_805F8280);
}

// 0x8001D3BC
// Deletes the last character of the name entry.
void NameDelLetter(void) {
    u8* name = lbl_805FAF78;
    if (name == NULL) return;
    s32 count = *(s32*)(name + 0xBF8);
    if (count > 0) {
        count -= 1;
        *(s32*)(name + 0xBF8) = count;
        *(u8*)(name + 0xAE8 + count) = 0;
    }
}

}  // extern "C"
