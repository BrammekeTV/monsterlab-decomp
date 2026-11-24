// Small functions: fix-up callbacks after load, frame effects, camera freeze, friend code entry,
// friends list (0x8004FE40 - 0x801DB7E0).
#include <string.h>

#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
u32   AlertGetLastError(void);                                       // 0x80140B58
void  MatchupSetHighlight(void* panel, u32 hash, BOOL on);           // 0x8013FBCC
void  MatchupSetReady(BOOL ready);                                   // 0x8013F2A4
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);       // 0x80209E70
void* ScreenSetFindWidget(void* set, u32 hash);                      // 0x80198E34
void* ScreenSetsCurrent(void* sets);                                 // 0x8018D1E4
int   SysSprintf(char* buf, const char* fmt, ...);                   // 0x80142EFC
void  UISetText(void* el, const char* text, s32 a);                  // 0x8019EFC8
void* UIWidgetFindChild(void* widget, u32 hash);                     // 0x80199AEC
void  fn_80116D40(u8* p, u32 a, u32 b);                              // 0x80116D40
void  fn_80116E54(u8* p, s32 a, s32 b);                              // 0x80116E54 (return value unused here)
void  fn_80116EF0(u8* p, s32 a);                                     // 0x80116EF0
void fn_8012F818(u8* p);                                     // 0x8012F818
void  fn_8012F8B8(u8* p, s32 a1, s32 a2);                            // 0x8012F8B8
s32   fn_80139FF0(void);                                             // 0x80139FF0 (friend code verified flag == 1)
u64   fn_8013A004(void);                                             // 0x8013A004 (verified friend code, hi:lo)
char* fn_8013A010(void);                                             // 0x8013A010 (friend code display buffer)
void  GetFriendStatusString(s32 a, s32 b);                                     // 0x8013A0A4 (callback)
void  fn_8013A654(s32 a);                                            // 0x8013A654 (callback)
void  fn_8013A6F4(s32 a, s32 b);                                     // 0x8013A6F4 (callback)
void  fn_8013F394(s32 a);                                            // 0x8013F394
void  fn_80142470(void);                                             // 0x80142470
s32   fn_8016195C(void* p, s32 a, s32 b, void* c);                   // 0x8016195C
void  fn_801629D0(void* obj, s32 n);                                 // 0x801629D0
void*  fn_801BC574(void* p);                                          // 0x801BC574
void* fn_801D40E0(s32 n);                                            // 0x801D40E0
BOOL  fn_801F56FC(s32 flags);                                        // 0x801F56FC
void  fn_801F5714(s32 flags);                                        // 0x801F5714
void* fn_80209814(void* v);                                          // 0x80209814
void  OSLockMutex(u8* p);                                            // 0x8023A5B0
void  OSUnlockMutex(u8* p);                                            // 0x8023A68C
void  fn_802E65C0(u8* rec);                                          // 0x802E65C0 (clears a 12-byte friend record)
s32   fn_802E99C4(s32 a, void* cb1, s32 b, void* cb2, s32 c, void* cb3, s32 d);  // 0x802E99C4
s32   fn_802FA554(u8* rec);                                          // 0x802FA554 (record is a buddy)
s32   fn_802FA5BC(u8* rec, u64 code);                                // 0x802FA5BC
s32   fn_802FABA4(u8* rec);                                          // 0x802FABA4 (record slot is unused)
u64   fn_802FB094(u8* rec);                                          // 0x802FB094 (own friend code)
void  fn_802FB13C(u8* rec, u64 code);                                // 0x802FB13C (store a friend code)
s32   fn_8033861C(const char* s, const char* fmt, ...);              // 0x8033861C (sscanf)

extern "C++" void* gScreenSets;     // 0x805FB654
extern u8   gAlertPlayer;           ORIG(0x805F902C)
extern u8   lbl_80556AB0[];         // 0x80556AB0 (connection lock object)
extern u8   lbl_80556E80[];         // 0x80556E80 (player data block)
extern u8   lbl_80375C48[];         // 0x80375C48 (friend list hash tables at +0xD8/+0xE8/+0xF8/+0x108)
extern u32  lbl_803F0E30[];         // 0x803F0E30 (slot table)
extern u32  lbl_803F0F30[];         // 0x803F0F30 (flag table, indexed by slot table value)
extern u8   lbl_803F0CE0[];         // 0x803F0CE0 (typed digits, NUL terminated)
extern u8   lbl_803F0CF0[];         // 0x803F0CF0 (typed digits with dashes, NUL terminated)
extern u8   lbl_803F103C[];         // 0x803F103C (global object)
extern u8   lbl_803F2298[];         // 0x803F2298 (own friend code, dashed)
extern u8   lbl_8040E5F0[];         // 0x8040E5F0 (frame effects object)
extern s32  lbl_805FAE8C;           // 0x805FAE8C
extern u8   lbl_805FB1A4[];         // 0x805FB1A4 ([0] != 0: camera frozen)
extern s32  lbl_805FB338;           // 0x805FB338 (typed digit count)
extern s32  lbl_805FB33C;           // 0x805FB33C (typed text length incl. dashes)
extern u8   lbl_805FB340;           // 0x805FB340 (friend code verified)
extern s32  lbl_805FB348;           // 0x805FB348 (verified code, high word)
extern s32  lbl_805FB34C;           // 0x805FB34C (verified code, low word)
extern s32  lbl_805FB350;           // 0x805FB350 (scroll active)
extern s32  lbl_805FB35C;           // 0x805FB35C (list index)
extern s32  lbl_805FB360;           // 0x805FB360 (list scroll offset)
extern s32  lbl_805FB364;           // 0x805FB364
extern s32  lbl_805FB378;           // 0x805FB378
extern s32  lbl_805FB388;           // 0x805FB388 (invited code, high word)
extern s32  lbl_805FB38C;           // 0x805FB38C (invited code, low word)
extern s32  lbl_805FBB48;           // 0x805FBB48 (connection nesting counter)
extern void* lbl_805FBBAC;          // 0x805FBBAC (voice SFX handle)
extern f32  lbl_805F9010;           // 0x805F9010 (saved cursor position x)
extern f32  lbl_805F9014;           // 0x805F9014 (saved cursor position y)
extern f32  lbl_805F9018;           // 0x805F9018 (saved cursor position z)
extern char lbl_805F901C[];         // 0x805F901C (empty text)
extern s32  lbl_805F9028;           // 0x805F9028
}

typedef void (*ObjIntFn)(u8*, s32);

static const u32 kWidgetCodeEntry = 0x7D12C9EEu;   // friend code entry screen
static const u32 kWidgetCodeText  = 0x6C2E9B73u;   // typed text element
static const u32 kWidgetCursor    = 0x924D5809u;   // cursor element

// Friend record layout inside the player data block: 64 entries of 0xC bytes at +0x40 (codes),
// 64 entries of 0x20 bytes at +0x340 (name at +0, code hi/lo at +0x10/+0x14, flags at +0x1C).
static inline u64 ReadCode(const u8* e) {
    return ((u64)*(const u32*)(e + 0x350 - 0x340) << 32) | *(const u32*)(e + 0x354 - 0x340);
}

extern "C" {

// 0x8010C8C8
// Script callback (type 0x7): re-fixes the entity's parts (kind 0, slot from +0x1C). Returns true.
u32 FixUpEntityCallback(u32 a0, u32 a1) {
    fn_8012F8B8((u8*)a0, 0, *(s32*)((u8*)a1 + 0x1C));
    return 1;
}

// 0x8004FE40
// Script callback (type 0x7): re-fixes the entity part (kind 1). Returns true.
u32 FixUpEntityPartCallback(u32 a0, u32 a1) {
    fn_8012F8B8((u8*)a0, 1, 0);
    return 1;
}

// 0x8010C87C
// Script callback (type 0x7): re-fixes the pooled entity stored in the slot, if any. Returns true.
u32 FixUpPooledEntityCallback(u32 a0, u32 a1) {
    u8* ent = *(u8**)a0;
    if (ent != NULL) {
        fn_8012F8B8(ent, 1, 0);
    }
    return 1;
}

// 0x8010C758
// Script callback (type 0x7): re-fixes the render object stored in the slot. Returns true.
u32 FixUpRenderObjectCallback(u32 a0, u32 a1) {
    fn_8012F818(*(u8**)a0);
    return 1;
}

// 0x8010CCB0
// Script callback (type 0x7): calls fn_801BC574 on the object two pointer levels down the slot
// (when every level is non-null). Returns true.
u32 FixupFSMPoolCallback(u32 a0, u32 a1) {
    if (a0 != 0) {
        u8** pool = *(u8***)a0;
        if (pool != NULL) {
            u8* fsm = *pool;
            if (fsm != NULL) {
                fn_801BC574(fsm);
            }
        }
    }
    return 1;
}

// 0x801DB7E0
// Screen blur effect: level clamped to 1..3, strength to 0..1, duration to 0.1..10.
void FrameFxBlur(s32 a0, f32 a1, f32 a2) {
    struct { s32 level; f32 strength; f32 duration; } args;

    s32 level = 1;
    if (a0 >= 1) {
        level = (a0 <= 3) ? a0 : 3;
    }
    args.level = level;

    f32 strength = 0.0f;
    if (a1 >= 0.0f) {
        strength = (a1 <= 1.0f) ? a1 : 1.0f;
    }
    args.strength = strength;

    f32 duration = 0.1f;
    if (a2 >= 0.1f) {
        duration = (a2 <= 10.0f) ? a2 : 10.0f;
    }
    args.duration = duration;

    fn_801629D0(lbl_8040E5F0, 0);
    fn_8016195C(lbl_8040E5F0, 0, 0, &args);
}

// 0x801DB748
// Plays frame effect a0; effects 0..2 are stopped as a group first, otherwise only effect a0.
void FrameFxPlay(u32 a0) {
    s32 n = (s32)a0;
    if (n >= 0 && n < 3) {
        fn_801629D0(lbl_8040E5F0, 0);
        fn_801629D0(lbl_8040E5F0, 1);
        fn_801629D0(lbl_8040E5F0, 2);
    } else {
        fn_801629D0(lbl_8040E5F0, n);
    }
    fn_8016195C(lbl_8040E5F0, n, 0, NULL);
}

// 0x800F7808
// Script callback (type 0x67): freezes the camera when a0 > 0 (signed).
void FreezeCamera(u32 a0) {
    lbl_805FB1A4[0] = (s32)a0 > 0;
}

// 0x80139B10
// Friend code entry keypad: key 10 deletes the last digit, keys 0..9 append a digit (max 12,
// dash after the 4th and 8th). The text element is refreshed afterwards.
void FriendCodeInputClickedKey(BOOL a0) {
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySound(0xCA084594u, 0, 0, 0, 0, 0);

    if ((u32)a0 == 10) {
        // Backspace
        if (lbl_805FB338 != 0) {
            s32 pos = lbl_805FB33C;
            s32 count = lbl_805FB338 - 1;
            lbl_805FB338 = count;
            lbl_803F0CE0[count] = 0;
            if (lbl_803F0CF0[pos - 1] == '-') {
                pos -= 1;
                lbl_803F0CF0[pos] = 0;
            }
            lbl_805FB33C = pos - 1;
            lbl_803F0CF0[pos - 1] = 0;
        }
    } else if ((u32)a0 <= 9) {
        if ((u32)lbl_805FB338 < 12u) {
            u8 digit = (u8)"0123456789"[a0];
            s32 n = lbl_805FB338;
            s32 pos = lbl_805FB33C;
            lbl_803F0CE0[n] = digit;
            lbl_805FB338 = n + 1;
            lbl_803F0CE0[n + 1] = 0;
            lbl_803F0CF0[pos] = digit;
            pos += 1;
            lbl_805FB33C = pos;
            if (n + 1 == 4 || n + 1 == 8) {
                lbl_803F0CF0[pos] = '-';
                pos += 1;
                lbl_805FB33C = pos;
            }
            lbl_803F0CF0[pos] = 0;
        }
    } else {
        return;
    }

    void* set = ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        void* w = ScreenSetFindWidget(set, kWidgetCodeEntry);
        if (w != NULL) {
            void* text = UIWidgetFindChild(w, kWidgetCodeText);
            if (text != NULL) {
                UISetText(text, (const char*)lbl_803F0CF0, 1);
            }
        }
    }
}

// 0x801399EC
// Friend code entry screen init (skipped when the last alert is the 0x89CE136A one): clears the
// typed code, remembers the cursor element position once, clears the text.
void FriendCodeInputInit(void) {
    if (AlertGetLastError() == 0x89CE136Au) return;

    lbl_805FB338 = 0;
    lbl_803F0CE0[0] = 0;
    lbl_805FB33C = 0;
    lbl_803F0CF0[0] = 0;
    lbl_805FB340 = 0;
    lbl_805FB34C = 0;
    lbl_805FB348 = 0;

    void* set = ScreenSetsCurrent(gScreenSets);
    if (set != NULL) {
        void* w = ScreenSetFindWidget(set, kWidgetCodeEntry);
        if (w != NULL) {
            if (lbl_805F9010 == -1.0f && lbl_805F9014 == -1.0f && lbl_805F9018 == -1.0f) {
                u8* cursor = (u8*)UIWidgetFindChild(w, kWidgetCursor);
                if (cursor != NULL) {
                    lbl_805F9010 = *(f32*)(cursor + 0x68);
                    lbl_805F9014 = *(f32*)(cursor + 0x6C);
                    lbl_805F9018 = *(f32*)(cursor + 0x70);
                }
            }
            void* text = UIWidgetFindChild(w, kWidgetCodeText);
            if (text != NULL) {
                UISetText(text, lbl_805F901C, 1);
            }
        }
    }
    fn_8013F394(1);
    MatchupSetReady(FALSE);
    SetMPSection(0x89CE136Au);
    SetHitBoxes(TRUE);
}

// 0x80139CC4
// Friend code entry per-frame update: shows the cursor element and, when digits are typed,
// moves it (x) to the right end of the text element (text world position + 2.0).
void FriendCodeInputUpdate(void) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* w = (u8*)ScreenSetFindWidget(set, kWidgetCodeEntry);
    if (w == NULL) return;
    u8* text = (u8*)UIWidgetFindChild(w, kWidgetCodeText);
    u8* cursor = (u8*)UIWidgetFindChild(w, kWidgetCursor);
    if (cursor == NULL) return;
    if (text == NULL) return;

    (*(ObjIntFn*)(*(u8**)(cursor + 0x138) + 0x2C))(cursor, 2);

    if (strlen((const char*)lbl_803F0CF0) == 0) {
        *(f32*)(cursor + 0x68) = lbl_805F9010;
        *(f32*)(cursor + 0x6C) = lbl_805F9014;
        *(f32*)(cursor + 0x70) = lbl_805F9018;
        return;
    }

    // Transform the text element's local point (+0x1D4..) by its matrix (+0x17C..+0x1A8);
    // only x is used (the y/z results go to unused locals in the original).
    f32 x = *(f32*)(text + 0x1D4);
    f32 y = *(f32*)(text + 0x1D8);
    f32 z = *(f32*)(text + 0x1DC);
    f32 wx = *(f32*)(text + 0x1A0) + ((*(f32*)(text + 0x194) * z) + ((*(f32*)(text + 0x17C) * x) + (*(f32*)(text + 0x188) * y)));
    *(f32*)(cursor + 0x68) = 2.0f + wx;
    *(f32*)(cursor + 0x6C) = lbl_805F9014;
    *(f32*)(cursor + 0x70) = lbl_805F9018;
}

// 0x80139E54
// Friend code entry confirm: needs 12 digits. Own code or a code already rejected by the lib
// signals 0x94B1685F; a code already in the friends list signals 0xF719853A; otherwise the
// code is stored as the verified one and 0xD919921E is signalled.
void FriendCodeInputVerify(void) {
    if ((u32)lbl_805FB338 != 12u) {
        GameFlowSignal((s32)0x94B1685Fu, 0, 0);
        return;
    }
    OSLockMutex(lbl_80556AB0);
    lbl_805FBB48 += 1;

    u64 code;
    fn_8033861C((const char*)lbl_803F0CE0, "%012llu", &code);

    if (code == fn_802FB094(lbl_80556E80) || fn_802FA5BC(lbl_80556E80, code) == 0) {
        GameFlowSignal((s32)0x94B1685Fu, 0, 0);
    } else {
        for (s32 i = 0; i < 64; i++) {
            if (ReadCode(lbl_80556E80 + i * 0x20 + 0x340) == code) {
                GameFlowSignal((s32)0xF719853Au, 0, 0);
                OSUnlockMutex(lbl_80556AB0);
                lbl_805FBB48 -= 1;
                return;
            }
        }
        lbl_805FB340 = 1;
        lbl_805FB34C = (s32)(u32)code;
        lbl_805FB348 = (s32)(u32)(code >> 32);
        GameFlowSignal((s32)0xD919921Eu, 0, 0);
    }
    OSUnlockMutex(lbl_80556AB0);
    lbl_805FBB48 -= 1;
}

// 0x8013ABDC
// Deletes the selected friend: clears its record and name, saves the options, re-sorts the list
// and, when a pending invite (+0x800 flag) is no longer from any listed friend, cancels it.
void FriendListDelete(void) {
    OSLockMutex(lbl_80556AB0);
    u32 slot = lbl_803F0E30[lbl_805FB35C + lbl_805FB360];
    lbl_805FBB48 = lbl_805FBB48 + 1;

    u8* e = lbl_80556E80 + slot * 0x20;
    *(u8*)(e + 0x340) = 0;
    *(u32*)(e + 0x354) = 0;
    *(u32*)(e + 0x350) = 0;
    *(u32*)(e + 0x35C) = 0;
    *(u32*)(e + 0x358) = 0;
    lbl_803F0F30[slot] = 0;
    fn_802E65C0(lbl_80556E80 + slot * 0xC + 0x40);
    SaveManager_SaveOptions();
    fn_802E99C4(0, (void*)fn_8013A6F4, 0, (void*)GetFriendStatusString, 0, (void*)fn_8013A654, 0);
    FriendListSort(lbl_805F9028);
    OSUnlockMutex(lbl_80556AB0);
    lbl_805FBB48 -= 1;

    if (fn_801F56FC(0x800) != 0) {
        u64 invited = ((u64)(u32)lbl_805FB388 << 32) | (u32)lbl_805FB38C;
        bool found = false;
        for (s32 i = 0; i < 64; i++) {
            u64 c = ReadCode(lbl_80556E80 + i * 0x20 + 0x340);
            if (c != 0 && c == invited) {
                found = true;
                break;
            }
        }
        if (!found) {
            fn_801F5714(0x800);
            lbl_805FB38C = 0;
            lbl_805FB388 = 0;
            MatchupCleanup();
        }
    }
    GameFlowSignal((s32)0x30B41BCAu, 0, 0);
}

// 0x8013A9B0
// Friends list init (skipped when the last alert is 0xF5F0817A): resets list state, builds the
// player's own friend code as "XXXX-XXXX-XXXX", sets up the list scroller and sorts the list.
void FriendListInit(void) {
    OSLockMutex(lbl_80556AB0);
    lbl_805FBB48 += 1;
    if (AlertGetLastError() != 0xF5F0817Au) {
        lbl_805FAE8C = 0;
        lbl_805FB364 = 1;
        lbl_805FB378 = 0;
        lbl_805FB35C = 0;
        lbl_805FB360 = 0;
        lbl_805FB350 = 0;
        fn_80142470();

        char digits[16];
        SysSprintf(digits, "%012llu", fn_802FB094(lbl_80556E80));
        u8* out = lbl_803F2298;
        for (s32 i = 0; i < 12; i++) {
            if (i % 4 == 0 && i != 0) *out++ = '-';
            *out++ = (u8)digits[i];
        }
        *out = 0;

        fn_8013F394(1);
        MatchupSetReady(FALSE);
        *(u32*)(lbl_803F103C + 0x18) = 0xC792EC44u;
        fn_80116D40(lbl_803F103C, 0x0B0260DEu, 0x8891E387u);
        fn_80116E54(lbl_803F103C, 4, 0x40);
        fn_80116EF0(lbl_803F103C, lbl_805FB35C);
        FriendListSort(lbl_805F9028);
        SetMPSection(0xF5F0817Au);
        SetHitBoxes(TRUE);
    }
    OSUnlockMutex(lbl_80556AB0);
    lbl_805FBB48 -= 1;
}

// 0x8013A910
// Fills the 64-entry flag table (indexed by slot) with 0x10.
void FriendListPreInit(void) {
    for (s32 i = 0; i < 64; i++) {
        lbl_803F0F30[i] = 0x10;
    }
}

// 0x8013B4A8
// Selects list row a0 on the current page: hides the floaty, plays a click and records the
// friend slot's player index for alerts.
void FriendListSetSelectedSlot(BOOL a0) {
    FriendListHideFloaty(TRUE);
    if (lbl_805FBBAC == NULL) {
        void* v = fn_801D40E0(1);
        if (v != NULL) v = fn_80209814(v);
        lbl_805FBBAC = v;
    }
    PlaySound(0xFBD5E144u, 0, 0, 0, 0, 0);
    s32 base = lbl_805FB35C;
    lbl_805FB360 = a0;
    gAlertPlayer = (u8)lbl_803F0E30[base + a0];
}

// 0x8013C1A4
// Shows the four floaty elements of the selected row (hash tables at +0xD8/+0xE8/+0xF8/+0x108)
// and highlights the last one.
void FriendListShowFloaty(void) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL) return;
    u8* w = (u8*)ScreenSetFindWidget(set, 0x9DE0B3FEu);
    if (w == NULL) return;

    u8* a = (u8*)UIWidgetFindChild(w, *(u32*)(lbl_80375C48 + 0xD8 + lbl_805FB360 * 4));
    if (a == NULL) return;
    (*(ObjIntFn*)(*(u8**)(a + 0x138) + 0x2C))(a, 2);

    u8* b = (u8*)UIWidgetFindChild(w, *(u32*)(lbl_80375C48 + 0xE8 + lbl_805FB360 * 4));
    if (b != NULL) {
        (*(ObjIntFn*)(*(u8**)(b + 0x138) + 0x2C))(b, 2);
    }
    u8* c = (u8*)UIWidgetFindChild(w, *(u32*)(lbl_80375C48 + 0xF8 + lbl_805FB360 * 4));
    if (c != NULL) {
        (*(ObjIntFn*)(*(u8**)(c + 0x138) + 0x2C))(c, 2);
    }
    u8* hashes = lbl_80375C48 + 0x108;
    u8* d = (u8*)UIWidgetFindChild(w, *(u32*)(hashes + lbl_805FB360 * 4));
    if (d != NULL) {
        (*(ObjIntFn*)(*(u8**)(d + 0x138) + 0x2C))(d, 2);
        MatchupSetHighlight(w, *(u32*)(hashes + lbl_805FB360 * 4), TRUE);
    }
}

// 0x8013C464
// Adds the verified friend code to the first free friend record, with the display name,
// flag bit 0 set; saves the options.
void FriendsListAddFriend(void) {
    if (fn_80139FF0() == 0) return;

    OSLockMutex(lbl_80556AB0);
    lbl_805FBB48 += 1;
    u64 code = fn_8013A004();
    for (s32 i = 0; i < 64; i++) {
        if (fn_802FABA4(lbl_80556E80 + 0x40 + i * 0xC) == 0) {
            fn_802FB13C(lbl_80556E80 + i * 0xC + 0x40, code);
            u8* e = lbl_80556E80 + i * 0x20;
            strcpy((char*)(e + 0x340), fn_8013A010());
            *(u32*)(e + 0x354) = (u32)code;
            *(u32*)(e + 0x350) = (u32)(code >> 32);
            *(u32*)(e + 0x35C) = *(u32*)(e + 0x35C) | 1;
            break;
        }
    }
    SaveManager_SaveOptions();
    fn_802E99C4(0, (void*)fn_8013A6F4, 0, (void*)GetFriendStatusString, 0, (void*)fn_8013A654, 0);
    OSUnlockMutex(lbl_80556AB0);
    lbl_805FBB48 -= 1;
}

// 0x8013BCE8
// Script callback (type 0x2E): true when the selected list entry is a buddy.
ScriptValue FriendsListIsBuddy(void) {
    OSLockMutex(lbl_80556AB0);
    lbl_805FBB48 = lbl_805FBB48 + 1;
    u32 slot = lbl_803F0E30[lbl_805FB35C + lbl_805FB360];
    s32 buddy = fn_802FA554(lbl_80556E80 + slot * 0xC + 0x40) != 0;
    OSUnlockMutex(lbl_80556AB0);
    lbl_805FBB48 -= 1;
    return (ScriptValue)buddy;
}

// 0x8013A004
// The verified friend code as one 64-bit value (the Wii returns the high word in r3, the low word in r4).
u64 fn_8013A004(void) {
    return ((u64)(u32)lbl_805FB348 << 32) | (u32)lbl_805FB34C;
}

}  // extern "C"
