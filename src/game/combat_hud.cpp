// Combat HUD: the part card shown when the player highlights one of their monster's parts.
// Original: SetPartSelected 0x8004533C (auto_01_80006B40_text.s)
#include <stddef.h>

#include "types.h"
#include "game/part.h"
#include "game/script_callbacks.h"

struct StringTable {
    const char** strings;
    u8           pad[8];
    u32          count;     // 0x0C
};

// Element vtable (secondary, at +0x138): 0x2C show, 0x30 hide.
typedef void (*UIVisFn)(void* el, s32 mode);
static inline void UIShow(void* el) { (*(UIVisFn*)(*(u8**)((u8*)el + 0x138) + 0x2C))(el, 2); }
static inline void UIHide(void* el) { (*(UIVisFn*)(*(u8**)((u8*)el + 0x138) + 0x30))(el, 2); }
static inline void UISetVisible(void* el, BOOL v) { if (v) UIShow(el); else UIHide(el); }

// Stat rows to show for an action type: a key and one flag per stat (damage, defence, cost).
struct ActionDisplay {
    u32 key;
    u8  show[4];
};

extern "C" {
void*  ScreenSetsCurrent(void* sets);                          // 0x8018D1E4
void*  ScreenSetFindElement(void* set, u32 hash);                     // 0x80198F18
void   UISetText(void* el, const char* text, s32 a);           // 0x8019EFC8
void   UISetNumber(void* el, s32 n);                           // 0x8019F2A8
void   UISetNegativeNumber(void* el, s32 n);                   // 0x8019F2F4
void   UISetTexture(void* el, BOOL tex);                       // 0x801A11FC
void   UIPlayAnim(void* el, u32 anim);                         // 0x8019BE40
s32   SysSprintf(char* buf, const char* fmt, ...);            // 0x80142EFC
void*  FighterGetSlot(void* fighter, s32 slot);                // 0x8020E794
BOOL   fn_80212E28(u8* p);
PartInstance** FighterStatsGetPart(void* stats, s32 slot);     // 0x80213FE0
s32    FighterStatsGet(void* stats, s32 stat);                 // 0x80214950: 3 = energy
BOOL   FighterStatsHasStatus(PartInstance** p, s32 status);    // 0x80214EE0
ActionDisplay PartDbGetActionDisplay(GameDb* db, u32 key);     // 0x802165E8
s32    PartActionStat0(PartInstance* p, u32 actionKey);        // 0x8021DBE8
s32    PartActionStat1(PartInstance* p, u32 actionKey);        // 0x8021DE10
s32    PartActionCost(PartInstance* p, u32 actionKey, void* fighter);  // 0x8021E280
BOOL   CanAffordAction(s32 cost, s32 energy, u32 typeKey);     // 0x80048AB8
void   CombatHudRefreshActions(void);                          // 0x800476D8
}

extern void*        gScreenSets;     // 0x805FB654
extern u8*          gBattle;         ORIG(0x805FBBB8)
extern u8*          gSaveSystem;     ORIG(0x805FBBE0)
extern StringTable* gStringTable;    // 0x805FB910
extern StringTable* gStringTable2;   ORIG(0x805FB90C)

#define HUD_COMBAT          0x1B66AC94   // "UI_MSA_Combat_HUD"
#define STATUS_DISABLED     0xD

// ---- data ----
static u8  sHudLocked;                    ORIG(0x805FB099)
static u8  sHudEnabled = 1;               ORIG(0x805F865C)
s32        gSelectedPart = -1;            ORIG(0x805F8650)
s32        gHighlightedAction = -1;       ORIG(0x805F8654)
static u8  sDefectShown;                  ORIG(0x805FB0BC)
static u8  sEnhancementShown;             ORIG(0x805FB0BD)
static u8  sActionReady[2];               ORIG(0x805FB09C)
s32        gPartSlotState[10];            ORIG(0x803C52E0)

static const u32 kDisciplineIcon[3] = { 0xF25835CB, 0x2D8AF405, 0x92825F01 };   // 0x803589EC
static const u32 kDisciplineName[3] = { 0x49829AB1, 0x4AE8C892, 0x87B52DD8 };   // 0x803589F8
static const u32 kStatValue[2][2]   = { { 0x5C9C9652, 0xC595C7E8 }, { 0x5BF1524B, 0xC2F803F1 } };
static const u32 kStatIcon[2][2][4] = {                                          // 0x80358A50
    { { 0x93BC3894, 0xA67617D1, 0x436D6C3E, 0xAFE27087 }, { 0x1D333F77, 0x9FFB2B14, 0xCDE26BDD, 0x9E0A6A1A } },
    { { 0x17F6366E, 0xFB9A46DD, 0xC72762C4, 0x5BAD5494 }, { 0x9979318D, 0xC2177A18, 0x49A86527, 0x6A454E09 } },
};
static const u32 kActionName[2]       = { 0x1D5D8016, 0x8454D1AC };
static const u32 kCostTooHigh[2]      = { 0x4E53F084, 0xD75AA13E };   // 0x805F8620
static const u32 kActionButton[2]     = { 0x78103316, 0xE11962AC };   // 0x805F85F0
static const u32 kActionReadyEvent[2] = { 0xE99A3E86, 0x70936F3C };   // 0x805F8608
static const u32 kActionLostEvent[2]  = { 0x265F1C00, 0xBF564DBA };   // 0x805F8600
static const u32 kActionElem[2]       = { 0x8B5466F5, 0x125D374F };   // 0x805F85F8
static const u32 kActionAnimOff[2]    = { 0xB86B5439, 0x05A138F7 };   // 0x805F8610
static const u32 kActionAnimOn[2]     = { 0x76B42FFE, 0xEF5649FF };   // 0x805F8618

// Resolve a string id; bit 31 selects the secondary table.
static inline const char* LookupString(u32 id) {
    if (id == 0xFFFFFFFF) return "Not found";
    if ((id & 0x80000000) && gStringTable2 != NULL) {
        u32 index = id & 0x7FFFFFFF;
        if (gStringTable2->strings != NULL && index < gStringTable2->count) return gStringTable2->strings[index];
        return "";
    }
    if (gStringTable->strings != NULL && id < gStringTable->count) return gStringTable->strings[id];
    return id == 0xFFFFFFFF ? "NOT FOUND" : "";
}

static inline const PartAction& DbAction(s32 i) {
    return ((PartAction*)*(u8**)(gGameDb->data + 0x430))[i];
}
static inline const PartActionType& DbActionType(s32 i) {
    return ((PartActionType*)*(u8**)(gGameDb->data + 0x08))[i];
}

static inline BOOL InTutorialBattle(void) {
    return gBattle != NULL && *(s32*)(gBattle + 0xAD0) == 4;
}

// Defect / enhancement icon: show the texture and, the first time, the tutorial pop-up.
static void ShowPartModifier(void* set, u32 widget, s32 icon, BOOL tutorial, u32 nis,
                             u32 textureList, u8* shown, u32 showEvent, u32 hideEvent) {
    void* el = ScreenSetFindElement(set, widget);
    if (el == NULL) return;
    if (icon != -1) {
        if (tutorial && !InTutorialBattle() && !IsMultiPlayer()) {
            CombatHUDDisableInput(0xADBF3357);
            SendNISSignal(nis);
        }
        UISetTexture(el, UITextureList_GetTexture(textureList, icon));
        if (!*shown) {
            *shown = 1;
            UIEvent_SendToAllScreens(showEvent);
        }
    } else if (*shown) {
        *shown = 0;
        UIEvent_SendToAllScreens(hideEvent);
    }
}

// 0x8004533C
BOOL SetPartSelected(s32 slot) {
    if (sHudLocked) return FALSE;
    if (slot <= -1 || slot >= 5) return FALSE;

    void* set = ScreenSetsCurrent(gScreenSets);
    if (set == NULL || *(u32*)set != HUD_COMBAT) return FALSE;

    u8* fighter = *(u8**)(gBattle + 0xA00);
    void* stats = fighter + 0x20;
    if (fighter == NULL || stats == NULL || ((u32*)fighter)[slot] == 0) return FALSE;
    if (fn_80212E28((u8*)FighterGetSlot(fighter, slot))) return FALSE;
    if (!sHudEnabled) return FALSE;

    gSelectedPart = slot;
    PartInstance* part = *((PartInstance***)fighter)[slot];
    PartDef def;
    PartDbGetDef(&def, gGameDb, part->defIndex);
    s32 discipline = def.discipline;

    char buf[0x100];
    if (void* el = ScreenSetFindElement(set, 0x18A9D495)) {
        s32* hp = *(s32**)(((u8**)fighter)[gSelectedPart] + 0x28);
        SysSprintf(buf, "%d/%d", hp[0], hp[1]);
        UISetText(el, buf, 1);
    }

    // Discipline icon and the part name in that discipline's colour.
    for (s32 i = 0; i < 3; i++) {
        if (void* el = ScreenSetFindElement(set, kDisciplineIcon[i])) UISetVisible(el, i == discipline);
        void* name = ScreenSetFindElement(set, kDisciplineName[i]);
        if (name == NULL) continue;
        if (i != discipline) { UIHide(name); continue; }
        UIShow(name);
        PartDef d;
        PartDbGetDef(&d, gGameDb, part->defIndex);
        if (part->tier >= 1 && part->tier <= PART_TIERS)
            UISetText(name, LookupString(TextIdFromHash(d.tierName[part->tier - 1])), 1);
    }

    // Action names.
    PartAction actions[2] = { DbAction(def.action1), DbAction(def.action2) };
    for (s32 a = 0; a < 2; a++) {
        if (void* el = ScreenSetFindElement(set, kActionName[a]))
            UISetText(el, LookupString(TextIdFromHash(DbActionType(actions[a].type).nameHash)), 1);
    }

    PartInstance* p = *FighterStatsGetPart(stats, gSelectedPart);
    ShowPartModifier(set, 0xCDC803FB, *(s32*)((u8*)p + 0x20),
                     *(u32*)(gSaveSystem + 0x118) > 300 && !CheckGameProgressBitFlag(0x13),
                     0x67DFB6AA, 0x15182BF7, &sDefectShown, 0x96D26F95, 0x480CEFAF);
    p = *FighterStatsGetPart(stats, gSelectedPart);
    ShowPartModifier(set, 0x938AC70D, *(s32*)((u8*)p + 0x24),
                     !CheckGameProgressBitFlag(0x14),
                     0xC7395052, 0xFA143C1A, &sEnhancementShown, 0xB98AADF8, 0xDC945215);

    // Per action: up to two stat rows, then the cost / availability state.
    for (s32 a = 0; a < 2; a++) {
        const PartActionType& type = DbActionType(actions[a].type);
        ActionDisplay disp = PartDbGetActionDisplay(gGameDb, type.unk00[2]);
        s32 values[3];
        values[0] = PartActionStat0(part, actions[a].key);
        values[1] = PartActionStat1(part, actions[a].key);
        values[2] = PartActionCost(part, actions[a].key, fighter);

        s32 row = 0;
        for (s32 s = 0; s < 3; s++) {
            if (!disp.show[s]) continue;
            for (s32 j = 0; j < 4; j++)
                if (void* el = ScreenSetFindElement(set, kStatIcon[a][row][j])) UISetVisible(el, j == s);
            if (void* el = ScreenSetFindElement(set, kStatValue[a][row])) {
                if (values[s] < 0) UISetNegativeNumber(el, -values[s]);
                else UISetNumber(el, values[s]);
            }
            row++;
        }
        SysSprintf(buf, "");
        for (; row < 2; row++) {
            for (s32 j = 0; j < 4; j++)
                if (void* el = ScreenSetFindElement(set, kStatIcon[a][row][j])) UISetVisible(el, j == 3);
            if (void* el = ScreenSetFindElement(set, kStatValue[a][row])) UISetText(el, buf, 1);
        }

        s32 cost = PartActionCost(part, actions[a].key, fighter);
        PartInstance** sp = FighterStatsGetPart(stats, slot);
        BOOL usable = CanAffordAction(cost, FighterStatsGet(stats, 3), type.unk00[2]) &&
                      !(sp != NULL && FighterStatsHasStatus(sp, STATUS_DISABLED));
        if (usable) {
            if (void* el = ScreenSetFindElement(set, kCostTooHigh[a])) UIHide(el);
            if (!sActionReady[a]) {
                if (void* el = ScreenSetFindElement(set, kActionButton[a])) {
                    UIPlayAnim(el, 0x42CCEF98);
                    UIEvent_SendToAllScreens(kActionReadyEvent[a]);
                    sActionReady[a] = 1;
                }
            }
        } else {
            if (void* el = ScreenSetFindElement(set, kCostTooHigh[a]))
                UISetVisible(el, cost > FighterStatsGet(stats, 3));
            if (gHighlightedAction != -1) {
                UIElement_PlayUIAnim(kActionElem[gHighlightedAction], kActionAnimOff[gHighlightedAction]);
                gHighlightedAction = -1;
            }
            if (sActionReady[a]) {
                if (void* el = ScreenSetFindElement(set, kActionButton[a])) {
                    UIPlayAnim(el, 0x88125C0F);
                    UIEvent_SendToAllScreens(kActionLostEvent[a]);
                    sActionReady[a] = 0;
                }
            }
        }
    }

    for (s32 i = 0; i < 10; i++) gPartSlotState[i] = -1;
    gPartSlotState[slot] = 1;

    // Keep the highlighted action if the cursor is still over a usable one.
    for (s32 a = 0; a < 2; a++) {
        const PartActionType& type = DbActionType(actions[a].type);
        s32 cost = PartActionCost(part, actions[a].key, fighter);
        PartInstance** sp = FighterStatsGetPart(stats, slot);
        if (!UIElement_IsCursorOverItem(kActionElem[a])) continue;
        if (!CanAffordAction(cost, FighterStatsGet(stats, 3), type.unk00[2])) continue;
        if (sp != NULL && FighterStatsHasStatus(sp, STATUS_DISABLED)) continue;
        if (gHighlightedAction != a) {
            if (gHighlightedAction != -1)
                UIElement_PlayUIAnim(kActionElem[gHighlightedAction], kActionAnimOff[gHighlightedAction]);
            UIElement_PlayUIAnim(kActionElem[a], kActionAnimOn[a]);
        }
        SetActiveAction(a);
    }

    CombatHudRefreshActions();
    if (gSelectedPart != 4) SendNISSignal(0x1D81FBFA);
    return TRUE;
}
