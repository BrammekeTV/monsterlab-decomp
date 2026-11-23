// Miscellaneous functions from the fifth "missing" draft batch: combat/exploration HUD, character
// reaction and ground handling, lightning inventory, pad queries, waypoint path following, heap init.
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "game/character.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

struct StringTable;

typedef u8* (*VGetFn)(u8*);
typedef void (*VFnI)(void*, s32);
typedef void (*VFn)(void*);

static const u32 kHashCombatHud = 0x1B66AC94u;  // "UI_MSA_Combat_HUD"
static const u32 kHashActions = 0x3A0D0FF5u;    // "actions"
static const u32 kHashFollowWaypointPath = 0x80B4120Fu;  // "FollowWaypointPath"

extern "C" {
f32   AnimGetLength(void* anim);                                  // 0x8018C07C
void* ChoiceDestroy(void* c, s32 del);                            // 0x8020F394
void  CombatHudRefreshActions(void);                              // src/game/combat_hud.cpp
s32   GetRegion(void);                                            // 0x8001425C
s32   PlaySound(u32 hash, s32 a, s32 b, s32 c, s32 d, s32 e);    // 0x80209E70
void* ScreenSetFindElement(void* set, u32 hash);                  // 0x80198F18
void* ScreenSetFindWidget(void* set, u32 hash);                   // 0x80198E34
void* ScreenSetsCurrent(void* sets);                              // 0x8018D1E4
void  ScreenSetsFlush(void* sets);                                // flushes pending screen set events
u32   StringHash(const char* s);                                  // 0x801EB704
int   SysSprintf(char* buf, const char* fmt, ...);                // 0x80142EFC
s32 TriggerDoAction(void* action, void* a1, void* a2);          // 0x801E0054
void  UIPlayAnim(void* el, u32 anim);                             // 0x8019BE40
s32   WorldGetPhase(void* world);                                 // 0x800D83A8
void  fn_80016FD0(Character* c, int animate);                     // 0x80016FD0
void  fn_80041810(u8* dst, u8* src);                              // 0x80041810 (copy a choice)
void* TextureLoadShared(const char* name);                              // 0x80109330 (texture lookup by name)
void  fn_8018D370(const char* fmt, ...);                          // 0x8018D370 (debug log)
void* UIAnim_FindByName(void* set, u32 hash);                           // 0x80198F1C
s32 fn_8019BBA4(u8* arg0, u8* arg1, s32 arg2);               // 0x8019BBA4
s32 fn_8019CC7C(void* el, u32 flag);                            // 0x8019CC7C
u8*   fn_801B3308(void);                                          // 0x801B3308
void  fn_801B3310(u8* p, u8* q);                                  // 0x801B3310
void* fn_801B7198(void* anim, s32 a);                             // 0x801B7198
void* fn_801D40E0(s32 n);                                         // 0x801D40E0
s32   fn_800E2F10(u8* p, f32 dt);                                 // 0x800E2F10
s32   fn_801E7460(u8* table, u16* handle);                        // 0x801E7460 (is the handle live?)
void  fn_801E8D74(void* out, void* list, void* owner, void* anim);  // 0x801E8D74
s32 fn_801E9678(void* list, void* handle);                           // 0x801E9678
void* fn_801E9D18(void* p, u32 hash, s32 size);                   // 0x801E9D18
void  fn_801EAA5C(u8* anim);                                      // 0x801EAA5C
void  fn_801EAAEC(u8* anim, u8* rec, u8* obj, u8* col, s32 a5, s32 a6, s32 a7, s32 a8,
                  f32 f1, f32 f2, s32 s0, s32 s1, u8* s2, s32 s3, s32 s4, s32 s5);   // 0x801EAAEC
void  fn_801ED554(u8* out, u8* color);                            // 0x801ED554
void* fn_801EEB20(u8* heap, u32 size);                            // 0x801EEB20
void  fn_801EF070(u8* heap, void* p);                             // 0x801EF070
u8*   fn_801F8110(u8* p, s32 a4);                                 // 0x801F8110
BOOL  fn_80212E28(u8** slot);                                     // 0x80212E28
void  operator_delete(void* p);                                   // 0x801D426C
char* StrCopy(char* dst, const char* src);                        // 0x8022127C
}

extern u8   gGame[];                       // 0x803B53A0
extern "C++" void* gScreenSets;            // 0x805FB654
extern s32  gSelectedPart;                 // 0x805F8650
extern s32  gHighlightedAction;            // 0x805F8654
extern s32  gPartSlotState[];              // 0x803C52E0
extern StringTable* gStringTable;          // 0x805FB910
extern StringTable* gStringTable2;         // 0x805FB90C
extern const u32 lbl_803589D8[];           // 0x803589D8 (HUD element hashes per limb)
extern const u32 lbl_80358A90[];           // 0x80358A90 (exploration HUD element hashes, 5 per side)
extern const u32 lbl_80358AB8[];           // 0x80358AB8 (exploration HUD animation hashes by state)
extern const u32 lbl_80358AD8[];           // 0x80358AD8 (HUD element hashes)
extern const u32 lbl_805F85F8[];           // 0x805F85F8 (HUD element hashes)
extern u8   lbl_805F865C;                  // 0x805F865C
extern const char lbl_805F8200[];          // 0x805F8200 (empty string)
extern u8   lbl_80388E80[];                // 0x80388E80 (vtable)
extern s32  lbl_803C5268[];                // 0x803C5268 (exploration HUD fighter states, 5 per side)
extern u8   lbl_803C4298[];                // 0x803C4298
extern u8   lbl_80413400[];                // 0x80413400 (pointer-sized results)
extern u32  lbl_80413410[];                // 0x80413410 (pad button word, 4 channels)
extern u32  lbl_80413420[];                // 0x80413420 (pad button word, 4 channels)
extern u8*  lbl_805FAF78;                  // 0x805FAF78 (lightning manager)
extern u32  lbl_805FB0A0;                  // 0x805FB0A0 (last disabled HUD input hash)
extern u8   lbl_805FB0BC;                  // 0x805FB0BC
extern u8   lbl_805FB0BD;                  // 0x805FB0BD
extern u8   lbl_805FB0BE;                  // 0x805FB0BE (selection HUD visible)
extern u8   lbl_805FB1B8;                  // 0x805FB1B8 (CompoundTask hash cached)
extern const char* lbl_805FB258;           // 0x805FB258 (last enhancement text)
extern s32  lbl_805FB6B8;                  // 0x805FB6B8
extern u8   lbl_805FB710[];                // 0x805FB710 (color)
extern u8*  lbl_805FB8D8;                  // 0x805FB8D8 (animation slot table, 0x80 bytes per entry)
extern u8*  lbl_805FBBE0;                  // 0x805FBBE0 (save block pointer)
extern u32  lbl_806010F0;                  // 0x806010F0 (hash of "CompoundTask")

// Disables (flag 0) every HUD element whose hash is in the list.
static void HudDisableElements(void* set, const u32* hashes, s32 count) {
    for (s32 i = 0; i < count; i++) {
        void* el = ScreenSetFindElement(set, hashes[i]);
        if (el != NULL) fn_8019CC7C(el, 0);
    }
}

// Loads a string from a string table: pointer array at +0, entry count at +0xC.
static const char* StringTableEntry(StringTable* t, u32 idx, const char* fallback) {
    u8* tab = (u8*)t;
    const char** arr = *(const char***)(tab + 0x0);
    if (arr != NULL && idx < *(u32*)(tab + 0xc)) return arr[idx];
    return fallback;
}

extern "C" {

// 0x8004E3D0
// Scales the reaction animations of a character: finds the longest animation among the five
// slots (or the one the current choice names) and sets the speed ratio of each slot against it.
void CharacterStartReaction(u8* arg0) {
    if ((*(VGetFn*)(*(u8**)arg0 + 0xF0))(arg0) == NULL) return;
    if ((*(VGetFn*)(*(u8**)arg0 + 0xF0))(arg0) + 0x20 == NULL) return;

    u8 choice[12];
    fn_80041810(choice, (*(VGetFn*)(*(u8**)arg0 + 0xF0))(arg0) + 0x20);
    s32 limb = *(s32*)(choice + 8);
    ChoiceDestroy(choice, -1);

    f32 longest;
    if (limb == -1) {
        longest = 0.0f;
        for (s32 i = 0; i < 5; i++) {
            u8* slot = *(u8**)(arg0 + 0xC50 + i * 4);
            if (slot == NULL) continue;
            u8* anim = (*(VGetFn*)(*(u8**)slot + 0x10))(slot);
            if (anim != NULL && longest < AnimGetLength(anim)) {
                limb = i;
                longest = AnimGetLength(anim);
            }
        }
    }
    if (limb == -1) return;

    u8* chosen = *(u8**)(arg0 + 0xC50 + limb * 4);
    if (chosen == NULL) return;
    u8* chosenAnim = (*(VGetFn*)(*(u8**)chosen + 0x10))(chosen);
    f32 base = 0.0f;
    if (chosenAnim != NULL) base = AnimGetLength(chosenAnim);

    for (s32 i = 0; i < 5; i++) {
        u8* slot = *(u8**)(arg0 + 0xC50 + i * 4);
        if (slot == NULL) continue;
        u8* anim = (*(VGetFn*)(*(u8**)slot + 0x10))(slot);
        if (anim == NULL) continue;
        *(f32*)(*(u8**)(arg0 + 0xA0) + 0xB0) = AnimGetLength(anim) / base;
    }
}

// 0x8019BD18
// UI element event router: runs the action, anim or sound registered for event id *arg1.
// Each list is a chain of nodes {next, id, data...}; only the first match of each list fires.
s32 UIElement_PlayUIAnimByHash(u8* arg0, u32* arg1) {
    u32 ev = *arg1;

    for (u8* n = *(u8**)(arg0 + 0x4); n != NULL; n = *(u8**)n) {
        if (*(u32*)(n + 4) == ev) {
            TriggerDoAction(n + 8, NULL, NULL);
            break;
        }
    }
    for (u8* n = *(u8**)(arg0 + 0x8); n != NULL; n = *(u8**)n) {
        if (*(u32*)(n + 4) == ev) {
            u32 hash = *(u32*)(n + 8);
            void* anim = UIAnim_FindByName(ScreenSetsCurrent(gScreenSets), hash);
            if (anim == NULL) {
                fn_8018D370("UIElement : unable to find anim hash 0x%x descriptor ... \n", hash);
            }
            fn_8019BBA4(arg0, (u8*)anim, 10);
            break;
        }
    }
    for (u8* n = *(u8**)(arg0 + 0xC); n != NULL; n = *(u8**)n) {
        if (*(u32*)(n + 4) == ev) {
            PlaySound(*(u32*)(n + 8), 0, 0, 0, 0, 0);
            break;
        }
    }
    return 0;
}

// 0x8020F3D4
// Callback object constructor: sets the vtable, the owner (+8), clears two flag bytes and
// names the callback with a placeholder string.
u8* fn_8020F3D4(u8* arg0, s32 arg1) {
    *(s32*)(arg0 + 0x8) = arg1;
    *(u8*)(arg0 + 0x4) = 0;
    *(void**)arg0 = lbl_80388E80;
    *(u8*)(arg0 + 0x5) = 0;
    StrCopy((char*)(arg0 + 0xC), "NAME THIS CALLBACK!!!");
    return arg0;
}

// 0x8020F42C
// Deleting destructor of the callback object: frees it when del > 0.
u8* fn_8020F42C(u8* arg0, s32 arg1) {
    if (arg0 != NULL && arg1 > 0) {
        operator_delete(arg0);
    }
    return arg0;
}

// 0x8005A520
// Resolves the ground the character stands on. If the cached ground still has no pending
// flag, it is returned; otherwise the ground triggers its enter actions and the cache
// (+0x7B0 handle, +0x7B4 flag, two float vectors) is reset. Returns NULL after a reset.
u8* CharacterGetGround(u8* arg0) {
    s32 h = *(s32*)(arg0 + 0x7B0);
    u8* g = (u8*)fn_801B7198(&h, 0);
    if (g != NULL && *(s32*)(arg0 + 0x7B4) != 0 && *(s32*)(g + 0x8C) != 0) {
        return g;
    }

    s32 h2 = *(s32*)(arg0 + 0x7B0);
    u8* g2 = (u8*)fn_801B7198(&h2, 0);
    if (g2 != NULL) {
        if (*(s32*)(g2 + 0x8C) == 0) {
            *(s32*)(arg0 + 0x790) = 1;
            *(u32*)(arg0 + 0x838) |= 0x02000000u;
        }
        u8* t = *(u8**)(g2 + 0x9C);
        if (t != NULL && ((*(u32*)(t + 0xA4) >> 27) & 0xF) != 0) {
            TriggerDoAction(t + 0xA4, arg0, g2);
        }
        if (((*(u32*)(g2 + 0x74) >> 27) & 0xF) != 0) {
            TriggerDoAction(g2 + 0x74, arg0, g2);
        }
    }
    *(s32*)(arg0 + 0x7B4) = 0;
    *(s32*)(arg0 + 0x7B0) = lbl_805FB6B8;
    *(f32*)(arg0 + 0x7B8) = 0.0f;
    *(f32*)(arg0 + 0x7BC) = 0.0f;
    *(f32*)(arg0 + 0x7C0) = 0.0f;
    *(f32*)(arg0 + 0x7C4) = 0.0f;
    *(f32*)(arg0 + 0x7C8) = 0.0f;
    *(f32*)(arg0 + 0x7CC) = 0.0f;
    *(f32*)(arg0 + 0x7D0) = 1.0f;
    return NULL;
}

// 0x800486A0
// Greys out the combat HUD elements that belong to the input group a0 (remembered in
// lbl_805FB0A0). Only acts while the combat HUD screen set is current.
void CombatHUDDisableInput(u32 a0) {
    lbl_805FB0A0 = a0;
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL || *(u32*)set != kHashCombatHud) return;

    switch (a0) {
    case 0xADBF3357u:  // "AlL": all limb elements, then fall through to the action buttons
        HudDisableElements(set, lbl_803589D8, 5);
        lbl_805F865C = 0;
        // fall through
    case kHashActions:
        HudDisableElements(set, lbl_805F85F8, 2);
        break;
    case 0x24E68B02u:
        SetPartSelected(gSelectedPart);
        break;
    case 0xAD4C5703u:
        HudDisableElements(set, lbl_80358AD8, 3);
        break;
    case 0x3F401183u:
    case 0x6A3C953Du: {
        void* el = ScreenSetFindElement(set, a0);
        if (el != NULL) fn_8019CC7C(el, 0);
        break;
    }
    case 0x7B3219A5u: {
        HudDisableElements(set, lbl_803589D8, 5);
        void* el = ScreenSetFindElement(set, lbl_803589D8[4]);
        if (el != NULL) fn_8019CC7C(el, 1);
        SetPartSelected(4);
        break;
    }
    default:
        break;
    }
}

// 0x8004CD28
// Creates the two per-player control objects of a character (slots at +0xC64) if missing.
// With flag set the new object also gets a lower speed (+0x94) and gain (+0x3EC).
void ControlsLock(u8* arg0, s32 arg1) {
    for (s32 i = 0; i < 2; i++) {
        u8* slot = arg0 + i * 4;
        if (*(s32*)(slot + 0xC64) != 0) continue;

        u8* o = (u8*)fn_801D40E0(0x484);
        if (o != NULL) o = fn_801F8110(o, *(s32*)(lbl_803C4298 + 0x38));
        *(u8**)(slot + 0xC64) = o;
        *(s32*)(o + 0x98) = i;
        if (arg1 != 0) {
            *(f32*)(*(u8**)(slot + 0xC64) + 0x94) = 0.2f;
            *(f32*)(*(u8**)(slot + 0xC64) + 0x3EC) = 0.1f;
        }
        o = *(u8**)(slot + 0xC64);
        *(f32*)(o + 0x9C) = 0.0f;
        *(f32*)(o + 0xA0) = -1.0f;
        *(f32*)(o + 0xA4) = 0.0f;
        fn_801B3310(fn_801B3308(), *(u8**)(slot + 0xC64));
    }
}

// 0x800515A8
// Conversation start: in phase 2 refreshes the player prompt, in phase 1 asks the conversation
// HUD widget (hash 0xE7667FBE) to run its slot 0x2C with 2.
void ConversationShowHUDInfo(void) {
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        fn_80016FD0(*(Character**)(gGame + 0xD614), 1);
    } else if (WorldGetPhase(gGame + 0xD5F0) == 1) {
        void* sets = *(void**)((u8*)gScreenSets + 0x88);
        if (sets != NULL) {
            u8* w = (u8*)ScreenSetFindWidget(sets, 0xE7667FBEu);
            if (w != NULL) {
                (*(VFnI*)(*(u8**)(w + 0x138) + 0x2C))(w, 2);
            }
        }
    }
}

// 0x80116870
// Texture of an enhancement icon: "ui/msa_enhancements/enhancement_icon%03d" with the number
// from the record, falling back to the temp icon. Also stores the record's description
// text (looked up through the string tables) in lbl_805FB258.
void* EnhancementGetTexture(s32 arg0) {
    u8* rec = *(u8**)(gGameDb->data + 0x3610) + arg0 * 0x24;
    char numbered[0x100];
    char path[0x100];

    u32 textHash = *(u32*)(rec + 0x8);
    SysSprintf(numbered, "enhancement_icon%03d", (s32)(s8)*(u8*)(rec + 0x20));
    strcpy(path, "ui/msa_enhancements/");
    strcat(path, numbered);
    void* tex = TextureLoadShared(path);

    s32 id = TextIdFromHash(textHash);
    const char* text;
    if (id == -1) {
        text = "Not found";
    } else if ((id & 0x80000000) && gStringTable2 != NULL) {
        text = StringTableEntry(gStringTable2, (u32)id & 0x7FFFFFFFu, lbl_805F8200);
    } else {
        text = StringTableEntry(gStringTable, (u32)id, lbl_805F8200);
    }
    lbl_805FB258 = text;

    if (tex == NULL) {
        strcpy(path, "ui/msa_enhancements/");
        strcat(path, "enhance_icon_temp");
        tex = TextureLoadShared(path);
    }
    return tex;
}

// 0x80045138
// Updates the five exploration HUD fighter badges of one side: from each fighter's health
// ratio picks a state (4 = KO'd, 3 = low, 2 = hurt, 0 = full, 1 = other) and plays the badge
// animation when the state changed.
void ExplorationHudSetFighter(u8* arg0, u8* arg1, s32 arg2) {
    if (arg1 == NULL || arg0 == NULL) return;

    s32 off = arg2 * 0x14;
    s32* states = (s32*)((u8*)lbl_803C5268 + off);
    const u32* elems = (const u32*)((const u8*)lbl_80358A90 + off);

    for (s32 i = 0; i < 5; i++) {
        u8* fighter = *(u8**)(arg1 + i * 4);
        u8** slot = fighter != NULL ? (u8**)(fighter + 0x28) : NULL;
        f32 ratio = 0.0f;
        if (slot != NULL) {
            s32* hp = *(s32**)slot;
            ratio = (f32)hp[0] / (f32)hp[1];
        }

        s32 state;
        if (fn_80212E28(slot)) {
            state = 4;
        } else {
            u8* tune = *(u8**)(gGameDb->data + 0x99D0);
            if (ratio < *(f32*)(tune + 0xE4)) {
                state = 3;
            } else if (ratio < *(f32*)(tune + 0xDC)) {
                state = 2;
            } else {
                f32 d = ratio - 1.0f;
                state = (d > -0.0001f && d < 0.0001f) ? 0 : 1;
            }
        }

        if (state != states[i]) {
            void* el = ScreenSetFindElement(arg0, elems[i]);
            if (el != NULL) UIPlayAnim(el, lbl_80358AB8[state]);
            states[i] = state;
        }
    }
}

// 0x801B8734
// Script callback FollowWaypointPath: starts the owner following its waypoint path with a
// tinted animation. Does nothing but a virtual refresh (slot 0xB0) when the owner is flagged
// (+0x4 -> +0x20 bit 0x04000000). a1 is a pointer to a color (a pointer in the original
// 32-bit build, the header types it u32), a2 the speed factor.
void FollowWaypointPath(void* a0, u32 /*0x17*/ a1, f32 a2) {
    u8* obj = (u8*)a0;
    u8* col = (u8*)(uintptr_t)a1;

    u8 tmpl[0x80];                                 // animation template (cleared, then filled)
    memset(tmpl, 0, 0x64);
    *(u32*)(tmpl + 0x64) = 0;
    *(s32*)(tmpl + 0x68) = 0;
    *(f32*)(tmpl + 0x70) = 0.0f;
    *(u16*)(tmpl + 0x78) = 0xFFFF;
    *(u16*)(tmpl + 0x7A) = 0xFFFF;
    *(u16*)(tmpl + 0x7C) = 0xFFFF;
    *(s32*)(tmpl + 0x74) = 0;
    *(u8*)(tmpl + 0x7E) = 0;
    *(u8*)(tmpl + 0x7F) = 0;
    *(void**)(tmpl + 0x6C) = (void*)fn_800E2F10;
    if ((s8)lbl_805FB1B8 == 0) {
        lbl_806010F0 = StringHash("CompoundTask");
        lbl_805FB1B8 = 1;
    }
    *(u32*)(tmpl + 0x64) = lbl_806010F0;

    u8 tint[4];
    fn_801ED554(tint, col);
    u8 anim[0x80];
    fn_801EAA5C(anim);

    s32 r0 = *(s32*)(lbl_80413400 + 0x0);
    s32 r1 = *(s32*)(lbl_80413400 + 0x4);
    s32 r2 = *(s32*)(lbl_80413400 + 0x8);

    if (*(u32*)(*(u8**)(obj + 4) + 0x20) & 0x04000000u) {
        (*(VFn*)(*(u8**)obj + 0xB0))(obj);
        return;
    }

    u8* entry = (u8*)fn_801E9D18(obj + 0xFC, kHashFollowWaypointPath, 0xF8);
    if (entry != NULL) {
        u16 v = *(u16*)(entry + 0x7C);
        fn_801E9678(obj + 0xFC, &v);
    }
    *(u32*)(tmpl + 0x64) = kHashFollowWaypointPath;
    u8 handleBuf[4];
    fn_801E8D74(handleBuf, obj + 0xFC, obj, tmpl);
    u16 handle = *(u16*)handleBuf;
    u8* table = lbl_805FB8D8;
    u16 handleCopy = handle;
    u8* rec = fn_801E7460(table, &handleCopy) ? table + ((u32)(handle & 0xFF) << 7) : NULL;
    if (rec == NULL) return;

    u8* sub = *(u8**)(obj + 4);
    if (col[1] == *(u16*)(sub + 0x32)) {
        f32 t = 8.0f * a2;
        sub[0x3A] = (u8) * (u16*)(tint + 2);
        f32 v = 0.0f;
        if (!(t < 0.0f)) {
            v = 255.0f;
            if (!(t > 255.0f)) v = t;
        }
        *(u8*)(*(u8**)(obj + 4) + 0x3B) = (u8)(s32)v;
    }

    u8 tail[4];
    tail[0] = lbl_805FB710[0];
    tail[1] = lbl_805FB710[1];
    *(u16*)(tail + 2) = *(u16*)(lbl_805FB710 + 2);
    u8 head[4];
    head[0] = tint[0];
    head[1] = tint[1];
    *(u16*)(head + 2) = *(u16*)(tint + 2);

    fn_801EAAEC(anim, rec, obj, head, 1, r0, -1, 2, a2, 0.0f,
                r1, -1, tail, 1, r2, *(s32*)(lbl_80413400 + 0xC));
    *(u32*)(anim + 0x64) = 0x9028DA4Eu;
    u8 handleBuf2[4];
    fn_801E8D74(handleBuf2, rec, obj, anim);
    (*(VFn*)(*(u8**)obj + 0xB0))(obj);
}

// 0x801EF8DC
// Initializes a heap record (size 0x394) over the block at base+0x10, once: sets the
// block bounds and the "init" tag, then frees the whole block into the free list.
// Returns 1 when it initialized, 0 when the heap was already set up.
s32 HeapInit(u8* arg0, u8* arg1, u32 arg2) {
    u8* start = arg1 + 0x10;
    if (*(u32*)(arg0 + 0x390) == 0x696E6974u) return 0;

    memset(arg0, 0, 0x394);
    s32 size = (s32)arg2 - 0x10;
    u8* end = start + size;
    *(s32*)(arg0 + 0x380) = size;
    *(u8**)(arg0 + 0x37C) = start;
    *(u8**)(arg0 + 0x384) = end;
    *(u8**)(arg0 + 0x388) = end;
    *(u32*)(arg0 + 0x390) = 0x696E6974u;  // "init"
    fn_801EF070(arg0, fn_801EEB20(arg0, 1));
    return 1;
}

// 0x80046E00
// Hides the combat selection HUD: tells every screen set to hide the selection widgets,
// resets the part slot states and the highlighted action. Only while the combat HUD is current
// and the selection HUD is shown.
void HideSelectionHUD(void) {
    u8* set = (u8*)ScreenSetsCurrent(gScreenSets);
    if (set == NULL || *(u32*)set != kHashCombatHud) return;
    if (lbl_805FB0BE == 0) return;

    UIEvent_SendToAllScreens(0x6F5C293Au);
    UIEvent_SendToAllScreens(0xFDD89CF0u);
    UIEvent_SendToAllScreens(0x3D77D92Du);
    if (lbl_805FB0BC != 0) {
        lbl_805FB0BC = 0;
        UIEvent_SendToAllScreens(0x480CEFAFu);
    }
    if (lbl_805FB0BD != 0) {
        lbl_805FB0BD = 0;
        UIEvent_SendToAllScreens(0xDC945215u);
    }
    ScreenSetsFlush(gScreenSets);
    u8* el = (u8*)ScreenSetFindElement(set, 0x7A1060A7u);
    if (el != NULL) {
        (*(VFnI*)(*(u8**)(el + 0x138) + 0x30))(el, 2);
    }
    gHighlightedAction = -1;
    for (s32 i = 0; i < 10; i++) gPartSlotState[i] = -1;
    CombatHudRefreshActions();
    lbl_805FB0BE = 0;
}

// 0x801409A8
// Script callback IsThisPAL: true when the region is not 0 (NTSC).
ScriptValue IsThisPAL(void) {
    return GetRegion() != 0;
}

// 0x8001E49C
// Script callback LightningGetInventoryPartID: id of the part in the selected lightning inventory
// slot (looked up in the save block), or -1.
s32 LightningGetInventoryPartID(void) {
    u8* mgr = lbl_805FAF78;
    if (mgr == NULL) return -1;
    u8* inv = mgr + *(s32*)(mgr + 0xA80) * 0x1A4 + 0x8;
    if (*(s32*)(inv + 0x198) > 0) {
        s32 sel = *(s32*)(inv + 0x190);
        if (sel >= 0) {
            s32 part = *(s32*)(inv + sel * 4);
            if (part >= 0) {
                return **(s32**)(lbl_805FBBE0 + part * 4 + 0xA58);
            }
        }
    }
    return -1;
}

// 0x8001E44C
// Script callback LightningGetInventoryPos: inventory index stored in the selected slot, or -1.
s32 LightningGetInventoryPos(void) {
    u8* mgr = lbl_805FAF78;
    if (mgr == NULL) return -1;
    u8* inv = mgr + *(s32*)(mgr + 0xA80) * 0x1A4 + 0x8;
    s32 count = *(s32*)(inv + 0x198);
    if (count > 0) {
        s32 sel = *(s32*)(inv + 0x190);
        if (sel < count) {
            return *(s32*)(inv + sel * 4);
        }
    }
    return -1;
}

// 0x801B9B40
// Pad buttons newly pressed on channel arg0 (0..3), 0 for other channels.
u32 PadGetButtons(s32 arg0) {
    if (arg0 < 0 || arg0 >= 4) return 0;
    return lbl_80413420[arg0];
}

// 0x801B9B98
// Pad buttons currently held on channel arg0 (0..3), 0 for other channels.
u32 PadGetHeld(s32 arg0) {
    if (arg0 < 0 || arg0 >= 4) return 0;
    return lbl_80413410[arg0];
}

// 0x800D5850
// Bitmask of the exits (0..3) of the current path node that exist and are not blocked
// (blocked exits count when a != 0).
u32 PathGetExits(PathFollower* arg0, int arg1) {
    u8* node = *(u8**)(*(u8**)arg0 + 0x14);
    if (node == NULL) return 0;
    u32 mask = 0;
    for (s32 i = 0; i < 4; i++) {
        if (*(s32*)(node + 0x4 + i * 4) != 0 && (*(s32*)(node + 0x24 + i * 4) == 0 || arg1 != 0)) {
            mask |= 1u << i;
        }
    }
    return mask;
}

// 0x8022127C
// Tail call to strcpy.
char* StrCopy(char* dst, const char* src) {
    return strcpy(dst, src);
}

}  // extern "C"
