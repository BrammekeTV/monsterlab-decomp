// Tutorial / part-selection callbacks (LT_*) and object kill helpers (KillX), verified against asm.
#include "types.h"

extern "C" {
#include "game/script_callbacks.h"
}

void UpdateAllZoneEntities(void);  // 0x80010228 (C++ linkage, defined in game_flow.cpp)

extern "C" {
// Callees
s32  fn_8001BFC0(void);                       // 0x8001BFC0 (current part index, -1 when none)
s32  fn_8001E508(s32 idx);                    // 0x8001E508 (result returned to the script)
s32  fn_8006C664(u8* obj);                    // 0x8006C664
void fn_8006C66C(u8* obj, s32 state);         // 0x8006C66C
void fn_8006C6AC(u8* obj);                    // 0x8006C6AC
void fn_8006CB54(u8* out, u8* obj);           // 0x8006CB54 (writes a vec3)
void fn_800D5BD4(u8* rec, s32 junction);      // 0x800D5BD4
void fn_800D7288(s32 handle);                 // 0x800D7288
u8*  ObjDbLookup(void* db, s32 id);           // 0x800D39BC
u8*  ObjRefDtor(u8* ref, s32 flags);          // 0x800D50D0
s32  WorldGetPhase(void* world);              // 0x800D83A8
s32  PlaySoundAt(void* sound, int a, const void* pos, int b);  // 0x80209ABC
u8*  SaveGetCurrentMonster(u8* s);            // 0x8022E170
void LabReleaseMonster(u8* inv, u8* m);       // 0x80219908
s32  MonsterPartsAllowedAtRank(u8* m);        // part_instance.cpp (BOOL)
void fn_8020A474(s32 handle);                 // 0x8020A474
void* fn_801D40E0(s32 n);                     // 0x801D40E0
void* fn_80209814(void* v);                   // 0x80209814

// Globals
extern u8   gGame[];                          // 0x803B53A0
extern void* gObjDb;                          // 0x805FB180
extern u8*  lbl_805FAF78;                     // 0x805FAF78 (part-selection state pointer)
extern u8*  lbl_805FBBE0;                     // 0x805FBBE0 (save block pointer)
extern void* lbl_805FBBAC;                    // 0x805FBBAC (voice SFX handle)
extern f32  lbl_805FC894;                     // 0x805FC894 (float constant, 1.0f)
}

extern "C" {

// Part list entry t (state + slot*0x1A4 + 8): returns the first word of the save-block part
// record for the entry's current list item, or -1 when the list is empty or the item is unset.
static s32 ListEntryPartWord(u8* t) {
    s32 cnt = *(s32*)(t + 0x198);
    if (cnt > 0) {
        s32 pos = *(s32*)(t + 0x190);
        if (pos >= 0) {
            s32 pid = *(s32*)(t + pos * 4);
            if (pid >= 0) {
                return *(s32*)(*(u8**)(lbl_805FBBE0 + pid * 4 + 0xA58));
            }
        }
    }
    return -1;
}

// Stops the tutorial voice SFX (state + 0xC44 handle) and clears the slot.
static void StopTutorialVoiceSlot(u8* hl) {
    u8* h = *(u8**)(hl + 0xC44);
    if (h != NULL) {
        fn_8020A474(*(s32*)h);
        *(u8**)(hl + 0xC44) = NULL;
    }
}

// 0x800FB490
// Kills an entity: calls its virtual at +0x58, then sets the 0x80 flag on its sub-record at +0x4.
void KillEntity(void* a0) {
    u8* obj = (u8*)a0;
    if (obj != NULL) {
        fn_800D7288(*(s32*)(obj + 0x2C));
        (*(void (**)(u8*))(*(u8**)obj + 0x58))(obj);
        u8* sub = *(u8**)(obj + 0x4);
        *(s32*)(sub + 0x20) = *(s32*)(sub + 0x20) | 0x80;
    }
}

// 0x8001D3F4
// Clears the highlight state of the five part slots and refreshes the zone entities.
void KillHighlights(void) {
    u8* hl = lbl_805FAF78;
    if (hl == NULL) return;
    *(s32*)(hl + 0xA80) = -1;
    for (s32 i = 0; i < 5; i++) {
        u8* a = *(u8**)(*(u8**)hl + i * 4 + 0xC50);
        if (a != NULL) fn_8006C6AC(a);
        u8* b = *(u8**)(hl + i * 0x1A4 + 0x1A8);
        if (b != NULL) {
            fn_8006C6AC(b);
            f32 v[4];
            fn_8006CB54((u8*)v, b);
            v[3] = lbl_805FC894;
            u8* d = *(u8**)(b + 0x90);
            *(f32*)(d + 0x38) = v[0];
            *(f32*)(d + 0x3C) = v[1];
            *(f32*)(d + 0x40) = v[2];
            *(f32*)(d + 0x44) = v[3];
        }
    }
    UpdateAllZoneEntities();
}

// 0x800E9818
// When the world is in phase 2, copies the object at a1 and runs fn_800D5BD4 on it for junction a0.
void KillParticleAtJunction(u32 a0, u32 a1) {
    s32 ref[0x101];  // 0x404-byte object copy
    if (WorldGetPhase(gGame + 0xD5F0) == 2) {
        u8* rec = ObjDbLookup(gObjDb, (s32)a1);
        for (s32 i = 0; i < 0x101; i++) ref[i] = ((s32*)rec)[i];
        fn_800D5BD4((u8*)ref, (s32)a0);
        ObjRefDtor((u8*)ref, -1);
    }
}

// 0x8001C570
// True when none of the five part slots holds a highlighted (state 8) object.
s32 LT_AllPartsForStickTutorialSelected(void) {
    u8* hl = lbl_805FAF78;
    if (hl == NULL) return 0;
    for (s32 i = 0; i < 5; i++) {
        u8* p = *(u8**)(hl + i * 0x1A4 + 0x1A8);
        if (p != NULL && fn_8006C664(p) == 8) return 0;
    }
    return 1;
}

// 0x8001C6D8
// Stops the tutorial voice and resets the tutorial text to the default string.
void LT_ClearTutorialText(void) {
    u8* hl = lbl_805FAF78;
    if (hl != NULL) StopTutorialVoiceSlot(hl);
    UIText_SetString_TextID(0xF8F01FECu, 0x557u);
}

// 0x8001C730
// Stops the tutorial voice SFX only.
void LT_ConversationStopVoiceSFX(void) {
    u8* hl = lbl_805FAF78;
    if (hl != NULL) StopTutorialVoiceSlot(hl);
}

// 0x8001E2AC
// Deletes the currently selected monster: runs the slot-0x18 virtual on the selected part and
// on the sub-object at +0x940 for all five slots, releases the monster, and fixes the save's current pointer.
void LT_DeleteMonster(void) {
    u8* hl = lbl_805FAF78;
    if (hl == NULL) return;
    u8* sel = NULL;
    s32 idx = *(s32*)(hl + 0xAD8);
    if (idx >= 0) sel = *(u8**)(lbl_805FBBE0 + idx * 4 + 0xA30);
    for (s32 i = 0; i < 5; i++) {
        if (sel != NULL) (*(void (**)(u8*, s32))(*(u8**)sel + 0x18))(sel, i);
        u8* sub = hl + 0x940;
        (*(void (**)(u8*, s32))(*(u8**)sub + 0x18))(sub, i);
    }
    s32 isCurrent = (sel == SaveGetCurrentMonster(lbl_805FBBE0));
    if (sel != NULL) LabReleaseMonster(lbl_805FBBE0 + 0xA30, sel);
    if (isCurrent) {
        if (*(s32*)(lbl_805FBBE0 + 0x10E8) != 0) {
            *(s32*)(lbl_805FBBE0 + 0x114) = *(s32*)(lbl_805FBBE0 + 0xA30);
            return;
        }
        *(s32*)(lbl_805FBBE0 + 0x114) = 0;
    }
}

// 0x8001C7B0
// Returns the save inventory index of the part at the current selection, or -1.
s32 LT_GetCurrentPartInventoryIndex(void) {
    u8* hl = lbl_805FAF78;
    if (hl != NULL) {
        s32 cur = *(s32*)(hl + 0xA80);
        if (cur != -1) {
            u8* t = hl + cur * 0x1A4 + 8;
            s32 cnt = *(s32*)(t + 0x198);
            if (cnt > 0) {
                s32 pos = *(s32*)(t + 0x190);
                if (pos < cnt) return *(s32*)(t + pos * 4);
            }
            return -1;
        }
    }
    return -1;
}

// 0x8001C500
// Highlights (state 8) every filled part slot, then refreshes the zone entities.
void LT_HighlightAllPartsForStickTutorial(void) {
    u8* hl = lbl_805FAF78;
    if (hl != NULL) {
        for (s32 i = 0; i < 5; i++) {
            u8* p = *(u8**)(hl + i * 0x1A4 + 0x1A8);
            if (p != NULL) fn_8006C66C(p, 8);
        }
    }
    UpdateAllZoneEntities();
}

// 0x8001C104
// True when a monster is being edited: selected index is valid and below the save's monster count.
s32 LT_IsEdittingMonster(void) {
    u8* hl = lbl_805FAF78;
    if (hl != NULL) {
        s32 m = *(s32*)(hl + 0xAD8);
        if (m >= 0 && m < *(s32*)(lbl_805FBBE0 + 0x10E8)) return 1;
    }
    return 0;
}

// 0x8001C37C
// Stores the current body part index at +0xA84.
void LT_MarkCurrentBodyPart(void) {
    u8* hl = lbl_805FAF78;
    if (hl != NULL) *(s32*)(hl + 0xA84) = fn_8001BFC0();
}

// 0x8001C440
// True when all five part slots have a usable part in the save.
s32 LT_MonsterHasAllParts(void) {
    u8* hl = lbl_805FAF78;
    if (hl == NULL) return 0;
    for (s32 i = 0; i < 5; i++) {
        u8* t = hl + 8 + i * 0x1A4;
        if (ListEntryPartWord(t) == -1) return 0;
    }
    return 1;
}

// 0x8001C13C
// True when the required slot pair (+0x698 and +0x1AC) both have usable parts.
s32 LT_MonsterHasRequiredParts(void) {
    u8* hl = lbl_805FAF78;
    if (hl == NULL || *(s32*)(hl + 0) == 0) return 0;
    if (ListEntryPartWord(hl + 0x698) == -1) return 0;
    return ListEntryPartWord(hl + 0x1AC) != -1;
}

// 0x8001C778
// True when the given part slot equals the current body part selection.
s32 LT_PartSelectionMatches(u32 a0) {
    return (s32)a0 == fn_8001BFC0();
}

// 0x8001C3CC
// True when the given part slot holds a usable part.
s32 LT_PartSlotHasValidPart(u32 a0) {
    u8* hl = lbl_805FAF78;
    if (hl == NULL) return 0;
    return ListEntryPartWord(hl + (s32)a0 * 0x1A4 + 8) != -1;
}

// 0x8001C20C
// True when the player's monster state is set and all its parts are allowed at the current rank.
s32 LT_PlayerCanUseAllMonsterParts(void) {
    u8* hl = lbl_805FAF78;
    if (hl != NULL && *(s32*)hl != 0 && MonsterPartsAllowedAtRank(hl + 0x940) != 0) return 1;
    return 0;
}

// 0x8001C5F4
// Sets the tutorial text, stops the previous tutorial voice and plays the new voice clip (unless flagged).
void LT_SetTutorialText(u32 a0, u32 a1) {
    u8* hl = lbl_805FAF78;
    u8* snd = (u8*)(uintptr_t)a1;
    if (hl != NULL) StopTutorialVoiceSlot(hl);
    UIText_SetString_TextID(0xF8F01FECu, 0x557u);
    UIText_SetString_TextID(0xF8F01FECu, a0);
    if (hl != NULL && snd != NULL && !(*(u8*)(snd + 5) & 1)) {
        if (lbl_805FBBAC == NULL) {
            void* v = fn_801D40E0(1);
            if (v != NULL) v = fn_80209814(v);
            lbl_805FBBAC = v;
        }
        PlaySoundAt(snd, 0, NULL, 2);
        *(u8**)(hl + 0xC44) = snd;
    }
}

// 0x8001C084
// Plays the part-selection sound when the selected part is highlighted, then forwards the index to fn_8001E508.
s32 LT_UpdatePartSelectionForInput(void) {
    s32 idx = fn_8001BFC0();
    u8* hl = lbl_805FAF78;
    if (idx != -1 && hl != NULL) {
        u8* obj = *(u8**)(hl + idx * 0x1A4 + 0x1A8);
        if (obj != NULL && fn_8006C664(obj) == 8) GameFlowSignal(0x3977C22B, 0, 0);
    }
    return fn_8001E508(idx);
}

// 0x800F1A88
// Tail call: if the target is non-null, calls its virtual at +0x58 on it.
void KillProp(u32 a0) {
    u8* obj = (u8*)(uintptr_t)a0;
    if (obj == NULL) return;
    (*(void (**)(u8*))(*(u8**)obj + 0x58))(obj);
}

}  // extern "C"
