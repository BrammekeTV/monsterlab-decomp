// Lab books UI script callbacks: parts page actions and the creatures page (0x8011xxxx).
#include "types.h"
#include "game/part.h"

// The header declares the script callbacks with C linkage so the definitions below match.
extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
extern u8  lbl_805FB248[];                // 0x805FB248 (lab books section table pointer)
extern f32 lbl_805FECBC;                  // 0x805FECBC (default float for missing creature data)

u32  fn_8011C1D0(u8* p, s32 a);           // 0x8011C1D0
u32  fn_8011C1F8(u8* p, s32 a);           // 0x8011C1F8
void fn_8011C20C(u8* p, s32 a);           // 0x8011C20C
s32  fn_8011C22C(u8* p);                  // 0x8011C22C (return type assumed, value passed through)
s32  fn_8011C2B4(u8* p);                  // 0x8011C2B4 (return type assumed, value passed through)
u8   fn_8011AEE0(void);                   // 0x8011AEE0
u8*  fn_8011AFEC(u8* p);                  // 0x8011AFEC (returns the delete-string buffer)
f32  fn_801173E8(u8* p);                  // 0x801173E8
f32  fn_801173D0(u8* p);                  // 0x801173D0
f32  fn_801173A0(u8* p);                  // 0x801173A0
f32  fn_801173B8(u8* p);                  // 0x801173B8
f32  fn_80117400(u8* p);                  // 0x80117400
f32  fn_80117418(u8* p);                  // 0x80117418
void fn_80117844(u8* p);                  // 0x80117844
void fn_80117430(u8* p);                  // 0x80117430
s32  fn_80117948(void);                   // 0x80117948 (takes no arguments in the leaf body)
s32  fn_801179C0(u8* p);                  // 0x801179C0
void fn_801179C8(u8* p);                  // 0x801179C8
s32  fn_80117ABC(u8* p);                  // 0x80117ABC
}

extern "C" {

// StringHash constants (CRC-32 of the upper-cased name, see tools/hashes.py).
static const u32 kHashParts     = 0x9E14654Eu;  // "parts"
static const u32 kHashCreatures = 0xE5368B8Bu;  // "creatures"

typedef u8* (*HashMethod)(u8* self, u32 hash);

// Calls the virtual slot at byte offset `off` of obj with a hash argument; returns r3.
static u8* CallVirt(u8* obj, s32 off, u32 hash) {
    HashMethod fn = *(HashMethod*)(*(u8**)obj + off);
    return fn(obj, hash);
}

// lbl_805FB248 -> pointer -> +4 sub-object (NULL if the table is missing).
static u8* LabBooksSection(void) {
    u8* top = *(u8**)lbl_805FB248;
    if (top == 0) return 0;
    return *(u8**)(top + 4);
}

// Parts page object: section->vfunc[0x28](parts), or NULL.
static u8* PartsObj(void) {
    u8* sec = LabBooksSection();
    if (sec == 0) return 0;
    return CallVirt(sec, 0x28, kHashParts);
}

// Creatures page object: section->vfunc[0x28](creatures), or NULL.
static u8* CreaturesObj(void) {
    u8* sec = LabBooksSection();
    if (sec == 0) return 0;
    return CallVirt(sec, 0x28, kHashCreatures);
}

// 0x80112E84
// Returns whether the current parts page item is valid for the given index.
s32 Parts_IsValidPart(s32 a0) {
    u8* obj = PartsObj();
    if (obj == 0) return 0;
    return (s32)fn_8011C1D0(obj, a0);
}

// 0x80112F04
// Returns whether the part popup is open for the given index.
s32 Parts_IsPartPopupOn(s32 a0) {
    u8* obj = PartsObj();
    if (obj == 0) return 0;
    return (s32)fn_8011C1F8(obj, a0);
}

// 0x80112F84
// Opens or closes the part popup for the given index.
void Parts_SetPartPopup(s32 a0) {
    u8* obj = PartsObj();
    if (obj != 0) {
        fn_8011C20C(obj, a0);
    }
}

// 0x80113080
// Reports whether the texture of the parts page's current item has finished loading.
s32 Parts_IsTextureAsyncLoaded(void) {
    u8* obj = PartsObj();
    if (obj == 0) return 0;
    return (s32)fn_8011AEE0();
}

// 0x801130F0
// Returns the delete prompt text for the parts page's current part.
s32 Parts_GetDeleteString(void) {
    u8* obj = PartsObj();
    if (obj == 0) return 0;
    return (BOOL)(uintptr_t)fn_8011AFEC(obj);  // TODO: header return type cannot hold a pointer on x64
}

// 0x80113160
// Removes the currently selected part. The callee's result is returned (0 when no parts page).
BOOL Parts_RemoveCurrentPart(s32 a0) {
    (void)a0;
    u8* obj = PartsObj();
    if (obj == 0) return 0;
    return (BOOL)fn_8011C22C(obj);
}

// 0x801131C8
// Returns whether the currently selected part is fitted on the monster.
s32 Parts_IsCurrentPartOnMonster(void) {
    u8* obj = PartsObj();
    if (obj == 0) return 0;
    return fn_8011C2B4(obj);
}

// 0x80113340
// Current HP of the creature shown on the creatures page (default when none).
f32 Creatures_GetCurrentHP(void) {
    u8* obj = CreaturesObj();
    if (obj == 0) return lbl_805FECBC;
    return fn_801173E8(obj);
}

// 0x801133BC
// Current energy of the creature shown on the creatures page (default when none).
f32 Creatures_GetCurrentEnergy(void) {
    u8* obj = CreaturesObj();
    if (obj == 0) return lbl_805FECBC;
    return fn_801173D0(obj);
}

// 0x80113438
// Current attack of the creature shown on the creatures page (default when none).
f32 Creatures_GetCurrentAttack(void) {
    u8* obj = CreaturesObj();
    if (obj == 0) return lbl_805FECBC;
    return fn_801173A0(obj);
}

// 0x801134B4
// Current defense of the creature shown on the creatures page (default when none).
f32 Creatures_GetCurrentDefense(void) {
    u8* obj = CreaturesObj();
    if (obj == 0) return lbl_805FECBC;
    return fn_801173B8(obj);
}

// 0x80113530
// Current madness of the creature shown on the creatures page (default when none).
f32 Creatures_GetCurrentMadness(void) {
    u8* obj = CreaturesObj();
    if (obj == 0) return lbl_805FECBC;
    return fn_80117400(obj);
}

// 0x801135AC
// Maximum madness of the creature shown on the creatures page (default when none).
f32 Creatures_GetMaxMadness(void) {
    u8* obj = CreaturesObj();
    if (obj == 0) return lbl_805FECBC;
    return fn_80117418(obj);
}

// 0x80113628
// Initialises the creature entity data for the creatures page.
void Creatures_InitEntityData(void) {
    u8* obj = CreaturesObj();
    if (obj != 0) {
        fn_80117844(obj);
    }
}

// 0x8011369C
// Refreshes the monster image on the creatures page.
void Creatures_UpdateMonsterImage(void) {
    u8* obj = CreaturesObj();
    if (obj != 0) {
        fn_80117430(obj);
    }
}

// 0x8011378C
// Returns non-zero when the player has monsters in the inventory.
s32 Creatures_MonstersInInventory(void) {
    u8* obj = CreaturesObj();
    if (obj == 0) return 0;
    return fn_80117948();
}

// 0x80113808
// Returns non-zero when a creature is loaded for the creatures page.
s32 Creatures_GetCreatureLoaded(void) {
    u8* obj = CreaturesObj();
    if (obj == 0) return 0;
    return fn_801179C0(obj);
}

// 0x80113884
// Shifts the creature's position to account for its legs.
void Creatures_AdjustPositionForLegs(void) {
    u8* obj = CreaturesObj();
    if (obj != 0) {
        fn_801179C8(obj);
    }
}

// 0x801138F8
// Lab books section check forwarded to the creatures sub-object's helper (0x80117ABC).
s32 fn_801138F8(void) {
    u8* obj = CreaturesObj();
    if (obj == 0) return 0;
    return fn_80117ABC(obj);
}

}
