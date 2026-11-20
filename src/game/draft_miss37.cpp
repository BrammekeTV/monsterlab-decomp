#include "types.h"
extern "C" {
#include "game/script_callbacks.h"
}

// Dtor used by the 0x988-byte save slot array (fn_8022ED98 is declared with a u8* return in another file).
#define SAVE_SLOT_DTOR ((void (*)(void*, s32))fn_8022ED98)
#define SAVE_SLOT_CTOR ((void (*)(void))fn_8022ED60)

// save.cpp defines these with C++ linkage.
BOOL SaveFileExists(const char* dir, const char* file);  // 0x80011960
s32 SaveGameRead(const char* name, void* data, u32 size);  // asm also passes r6=1; the definition takes 3 args

extern "C" {

// Callee prototypes. Definitions live in other files; types follow those definitions.
u32 fn_8012006C(s32 a3, s32 a4);                 // 0x8012006C (leaf/auto_03_8011F410_text.cpp)
void fn_801205EC(u8* p, s32 arg1);                 // 0x801205EC
u32 fn_8012078C(s32 a3, s32 a4);                 // 0x8012078C (leaf/auto_03_8011F410_text.cpp)
u32 fn_801207B8(s32 a3, s32 a4);                 // 0x801207B8 (leaf/auto_03_8011F410_text.cpp)
u32 fn_801207D0(s32 a3, s32 a4);                 // 0x801207D0 (leaf/auto_03_8011F410_text.cpp)
void fn_80142870(s32 a3);                        // 0x80142870
void fn_800D81AC(void* w);                       // 0x800D81AC
void* fn_801D40E0(s32 n);                        // 0x801D40E0
void* fn_801E9D18(void* p, u32 hash, s32 size);  // 0x801E9D18
BOOL fn_801E7460(u8* base, u16* id);             // 0x801E7460
s32 fn_801E9678(void* list, void* handle);       // 0x801E9678
u8* fn_8022E3DC(u8* s);                          // 0x8022E3DC
u8* fn_8022ED60(u8* p);                          // 0x8022ED60 (slot array ctor)
u8* fn_8022ED98(u8* p, s32 del);                 // 0x8022ED98 (slot array dtor)
void fn_8022EDD8(u8* p);                         // 0x8022EDD8
void* fn_80123364(u8* p, s32 del);               // 0x80123364
void fn_8032F1E8(u8* arr, void (*ctor)(void), void (*dtor)(void*, s32), s32 size, s32 count);  // 0x8032F1E8
void fn_8032F358(void* obj, void* cb);           // 0x8032F358
void __destroy_arr(void* arr, void (*dtor)(void*, s32), u32 size, u32 count);  // 0x8032F2E0
s32 fn_80138380(u8* world, s32 slot);                        // 0x80138380
void OSReturnToDataManager(void);
void operator_delete(void* p);                   // 0x801D426C
u32 RandomU32(void);                             // 0x80143460
s32 AnimSetFindClip(void* set, s32 anim);        // 0x801AC2FC
void PropPlayAnim(void* prop, int a, int b, BOOL once);  // 0x80069350
void EventSendTo(void* mgr, u32 event, void* from, u32 arg, int a, int b);
void SaveEndQuest(u8* s, BOOL abandon);          // 0x802229D8
void SaveReset(u8* s);                           // 0x80222300
void LabInventoryReset(u8* inv);                 // lab_inventory.cpp (takes the inventory block)
s32 SaveCurrentScoreSlot(u8* s);                 // 0x802255FC

extern "C++" void* gEventMgr;
extern u8 gGame[];
extern u8* lbl_803C6A70[3];
extern u8* lbl_805FB248;
extern u8* lbl_805FB2D0;
extern u8* lbl_805FB8D8;
extern u8* lbl_805FBBE0;
extern u8 lbl_805FB2D5;
extern s32 lbl_805FB2C8;
extern s32 lbl_805F8A38;
extern const char lbl_805F8FD4[];
extern const char lbl_805F8FD8[];

// Shared recipe-popup check: vtable slot 0x28 of the object at lbl_805FB248+8, asked with hash 0x9C2C1CAF.
// Returns 0 when the table or the object is missing (the asm's null-check chain).
static s32 RecipePopupQuery(void) {
    u8* tbl = lbl_805FB248;
    u8* obj;
    if (tbl == NULL) return 0;
    obj = *(u8**)(tbl + 0x8);
    if (obj == NULL) return 0;
    return (*(s32 (**)(u8*, u32))(*(u8**)obj + 0x28))(obj, 0x9C2C1CAF);
}

// 0x80114D90
void Recipes_IncrementRow(s32 a0) {
    s32 r = RecipePopupQuery();
    if (r != 0) fn_801205EC((u8*)(uintptr_t)r, a0);
}

// 0x801158F4
// Returns the recipe-popup query result for a0 via fn_801207B8 (0 when the popup is off).
s32 Recipes_IsRecipePopupOn(s32 a0) {
    s32 r = RecipePopupQuery();
    if (r != 0) return (s32)fn_801207B8(r, a0);
    return 0;
}

// 0x80115868
// Returns the recipe validity result for a0 via fn_8012078C (0 when the popup query is off).
s32 Recipes_IsValidRecipe(s32 a0) {
    s32 r = RecipePopupQuery();
    if (r != 0) return (s32)fn_8012078C(r, a0);
    return 0;
}

// 0x801157E4
// Forwards a0 to fn_8012006C as the selected list item when the popup query is on.
void Recipes_RenderList_SetSelectedItem(s32 a0) {
    s32 r = RecipePopupQuery();
    if (r != 0) fn_8012006C(r, a0);
}

// 0x80115980
// Forwards a0 to fn_801207D0 to set the recipe popup when the popup query is on.
void Recipes_SetRecipePopup(s32 a0) {
    s32 r = RecipePopupQuery();
    if (r != 0) fn_801207D0(r, a0);
}

// 0x801BBB0C
// Fires the pending event stored at +0x24 (if any) to the event manager, then clears it.
void RequestComplete(void* ctx, u32 request, u32 arg) {
    u8* p = (u8*)ctx;
    s32 ev = *(s32*)(p + 0x24);
    if (ev != 0) {
        EventSendTo(gEventMgr, (u32)ev, *(void**)(p + 0x10), 0x41618D7Bu, (int)request, (int)arg);
        *(s32*)(p + 0x24) = 0;
    }
}

// 0x801B8D34
// Looks up the waypoint-path record at +0xFC, finds the matching entry and resumes it.
void ResumeFollowingWaypointPath(void* a0) {
    u8* rec;
    u8* base;
    u8* node;
    u8* item;
    u16 id;
    u16 key;

    rec = (u8*)fn_801E9D18((u8*)a0 + 0xFC, 0x80B4120Fu, 0x70);
    if (rec == NULL) return;
    id = *(u16*)(rec + 0x7C);
    base = lbl_805FB8D8;
    key = id;
    if (fn_801E7460(base, &key) != 0) {
        node = base + (((u32)id << 7) & 0x7F80);
    } else {
        node = NULL;
    }
    item = (u8*)fn_801E9D18(node, 0x20CE505Du, 0xF8);
    if (item != NULL) {
        u16 key2 = *(u16*)(item + 0x7C);
        fn_801E9678(node, &key2);
    }
}

// 0x801DB494
// Empty function body: returns its first argument unchanged (blr).
s32 Return(s32 a0) {
    return a0;
}

// 0x800931C8
// Picks a random index in 0..a0 that differs from the three robot-slot entries; -1 when a0 < 0.
void RobotEvolutionExperiment_RobotGetUniqueRandomZeroToMax(s32 a0) {
    u32 n;
    u32 again;
    if (a0 < 0) {
        lbl_805F8A38 = -1;
        return;
    }
    n = (u32)a0 + 1u;
    do {
        s32 r = (s32)(RandomU32() % n);
        lbl_805F8A38 = r;
        again = 0;
        if (r == *(s32*)(lbl_803C6A70[0] + 0x44)) again = 1;
        if (r == *(s32*)(lbl_803C6A70[1] + 0x44)) again = 1;
        if (r == *(s32*)(lbl_803C6A70[2] + 0x44)) again = 1;
    } while (again != 0);
}

// 0x800937C0
// Finds the robot-slot entry whose id matches a0->chr and plays its looping animation clip.
void RobotEvolutionExperiment_RobotPlayLoopingAnimSound(ScriptCtx* a0) {
    u8* found = NULL;
    s32 i;
    for (i = 0; i < 3; i++) {
        if (*(u32*)(lbl_803C6A70[i] + 0x4) == *(u32*)((u8*)a0 + 0x10)) {
            found = lbl_803C6A70[i];
            break;
        }
    }
    if (found != NULL) {
        s32 anim = *(s32*)(found + 0x30);
        if (anim != 0 && (u32)anim + 0x606D0000u != 0x0000FF74u) {
            u8* prop = *(u8**)(found + 0x4);
            u8* set = *(u8**)(prop + 0x9C);
            if (set != NULL) {
                u8* clips = *(u8**)(set + 0x48);
                if (clips != NULL) {
                    s32 clip = AnimSetFindClip(clips, anim);
                    PropPlayAnim(*(void**)(found + 0x4), clip, 0, 1);
                }
            }
        }
    }
}

// 0x8012375C
// Returns whether a save file exists and its first slot is flagged in use (reads the save into a local buffer).
u32 SaveManager_CanLoadGame(void) {
    u8 buf[0x31C0];
    u8 flag;
    if (SaveFileExists(lbl_805F8FD4, lbl_805F8FD4) == 0 || lbl_805FB2D5 != 0) {
        return 0;
    }
    fn_8032F1E8(buf + 0xB98, SAVE_SLOT_CTOR, SAVE_SLOT_DTOR, 0x988, 4);
    SaveGameRead(lbl_805F8FD8, buf, 0x31C0);
    flag = buf[0xE00];
    __destroy_arr(buf + 0xB98, SAVE_SLOT_DTOR, 0x988, 4);
    return (s8)flag != 0;
}

// 0x80123818
// Returns 1 when the current save block reports a flag and saving is not blocked (lbl_805FB2D5 clear).
u32 SaveManager_CanSaveGame(void) {
    u32 r = 0;
    if ((s8)*fn_8022E3DC(lbl_805FBBE0) != 0 && lbl_805FB2D5 == 0) {
        r = 1;
    }
    return r;
}

// 0x80124340
// Destroys the temporary save data block (slot array and buffer) and clears the pointer.
void SaveManager_FreeTmpData(void) {
    u8* tmp = lbl_805FB2D0;
    if (tmp != NULL) {
        __destroy_arr(tmp + 0xB98, SAVE_SLOT_DTOR, 0x988, 4);
        operator_delete(tmp);
    }
    lbl_805FB2D0 = NULL;
}

// 0x80123C94
// Loads the save file into a local buffer and copies the current slot's data into the game state.
void SaveManager_LoadData(void) {
    u8 buf[0x31C0];
    s32 t;
    if (lbl_805FB2D5 != 0) return;
    fn_80142870(0);
    fn_8032F1E8(buf + 0xB98, SAVE_SLOT_CTOR, SAVE_SLOT_DTOR, 0x988, 4);
    SaveGameRead(lbl_805F8FD8, buf, 0x31C0);
    fn_8022EDD8(buf + 0xB98 + lbl_805FB2C8 * 0x988);
    t = *(s32*)(lbl_805FBBE0 + 0x6A8);
    if (t != 0) {
        *(s32*)(gGame + 0xD5F0) = t;
        fn_800D81AC(gGame + 0xD5F0);
    }
    fn_80142870(1);
    __destroy_arr(buf + 0xB98, SAVE_SLOT_DTOR, 0x988, 4);
}

// 0x8012423C
// Loads the save file into a freshly allocated temporary block (kept in lbl_805FB2D0).
void SaveManager_LoadDataToTmp(void) {
    u8* t;
    if (lbl_805FB2D5 != 0) return;
    if (lbl_805FB2D0 != NULL) {
        fn_8032F358(lbl_805FB2D0, (void*)fn_80123364);
    }
    t = (u8*)fn_801D40E0(0x31C0);
    if (t != NULL) {
        fn_8032F1E8(t + 0xB98, SAVE_SLOT_CTOR, SAVE_SLOT_DTOR, 0x988, 4);
    }
    lbl_805FB2D0 = t;
    SaveGameRead(lbl_805F8FD8, t, 0x31C0);
}

// 0x80123D60
// Ends the current quest, resets the save block and resets its inventory.
void SaveManager_ResetCurrentData(void) {
    SaveEndQuest(lbl_805FBBE0, 1);
    SaveReset(lbl_805FBBE0);
    LabInventoryReset(lbl_805FBBE0 + 0xA30);
}

// 0x80123868
// Scans the save file's four slots for the first unused one; stores its index in lbl_805FB2C8.
s32 SaveManager_SelectFirstEmptySlot(void) {
    u8 buf[0x31C0];
    u8* p;
    s32 i;
    if (SaveFileExists(lbl_805F8FD4, lbl_805F8FD4) == 0) {
        lbl_805FB2C8 = 0;
        return 1;
    }
    fn_8032F1E8(buf + 0xB98, SAVE_SLOT_CTOR, SAVE_SLOT_DTOR, 0x988, 4);
    SaveGameRead(lbl_805F8FD8, buf, 0x31C0);
    p = buf;
    for (i = 0; i < 4; i++) {
        if ((s8)p[0xE00] == 0) {
            lbl_805FB2C8 = i;
            __destroy_arr(buf + 0xB98, SAVE_SLOT_DTOR, 0x988, 4);
            return 1;
        }
        p += 0x988;
    }
    __destroy_arr(buf + 0xB98, SAVE_SLOT_DTOR, 0x988, 4);
    return 0;
}

// 0x801233CC
// Returns to the data manager (tail call).
void SaveManager_ReturnToMainMenu(void) {
    OSReturnToDataManager();
}

// 0x80131C28
// Releases the zone slot (tail call).
s32 ReleaseZoneSlot(u8* world, s32 slot) {
    return fn_80138380(world, slot);
}

// 0x802255FC
// Maps the 0..35 score-type code at +0x108 to a save score slot 0..5; -1 when out of range or unmapped.
s32 SaveCurrentScoreSlot(u8* s) {
    // Index is the jump-table case; values are the slot each case loads into r3 (-1 for the default).
    static const s8 kSlotByType[36] = {
        0, 1, 2, 3, -1, 4, 5,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 1, 2, 3, -1, 4, 5,
    };
    u32 idx = *(u32*)(s + 0x108);
    if (idx > 0x23) return -1;
    return kSlotByType[idx];
}

}  // extern "C"
