// Battle: which actions a fighter can use right now, the computer opponent's random pick, and the
// destructors of the fighter status and limb status objects.
// Original: 0x80213DC4, 0x8021415C, 0x802147E0, 0x802149AC
#include "types.h"
#include "game/part.h"

extern "C" {
void  operator_delete(void* p);                           // 0x801D426C
void* ChoiceDestroy(void* c, s32 del);                    // 0x8020F394
void  ValueBoxDestroy(void* box, s32 del);                // 0x80213CE0
void  EffectDestroy(u8* e, s32 del);                      // 0x80216318
void  LimbEffectDestroy(u8* e, s32 del);                  // 0x802165A0
void  EffectExpire(u8* e);                                // 0x802164A8
BOOL  FighterStatsHasStatus(PartInstance** p, s32 status);   // 0x80214EE0
BOOL  fn_80212E28(u8** slot);                             // 0x80212E28 (reaction_names.cpp)
s32   PartActionCost(PartInstance* p, u32 actionKey, void* fighter);  // 0x8021E280
s32   RandomUpTo(s32 max);
void  fn_8020F384(void* choice, s32 action, s32 flag, s32 limb);   // choice init
}
extern u8* gBattle;     // 0x805FBBB8

struct ActionOption { u8* limb; s32 action; };   // a usable limb and the action key it can run

extern "C" {

// 0x80213DC4
// Destroy a fighter's status object: its 8 effect slots (+0x28) are expired and freed, then its
// choice (+0x18) and two value boxes (+0x0C, +0x04).
u8* FighterStatsDestroy(u8* stats, s32 del) {
    if (stats != NULL) {
        for (s32 i = 0; i < 8; i++) {
            u8* e = *(u8**)(stats + 0x28 + i * 4);
            if (e != NULL) {
                EffectExpire(e);
                EffectDestroy(*(u8**)(stats + 0x28 + i * 4), 1);
                *(u8**)(stats + 0x28 + i * 4) = NULL;
            }
        }
        ChoiceDestroy(stats + 0x18, -1);
        ValueBoxDestroy(stats + 0xC, -1);
        ValueBoxDestroy(stats + 0x4, -1);
        if (del > 0) operator_delete(stats);
    }
    return stats;
}

// 0x802149AC
// Destroy a limb status object: 3 effects (+0x08), the limb effect (+0x14) and its value box.
u8* fn_802149AC(u8* obj, s32 del) {
    if (obj != NULL) {
        for (s32 i = 0; i < 3; i++) {
            u8* e = *(u8**)(obj + 8 + i * 4);
            if (e != NULL) {
                EffectDestroy(e, 1);
                *(u8**)(obj + 8 + i * 4) = NULL;
            }
        }
        if (*(u8**)(obj + 0x14) != NULL) {
            LimbEffectDestroy(*(u8**)(obj + 0x14), 1);
            *(u8**)(obj + 0x14) = NULL;
        }
        ValueBoxDestroy(obj, -1);
        if (del > 0) operator_delete(obj);
    }
    return obj;
}

// 0x8021415C
// List the (limb, action) pairs a fighter can use now into `out`, returning how many. `stats` is
// the fighter's status block (fighter + 0x20). Each working limb with a part offers its two
// actions if the energy cost fits what the fighter has; an action of type kind 5 needs the
// fighter's byte +0x18, and a kind-3 action is not offered while an effect 14 is active. If
// nothing fits, action 0x136 on the object at fighter +0x10 is offered.
s32 BattleListActions(u8* stats, ActionOption* out) {
    u8* fighter = NULL;
    if (gBattle != NULL) {
        u8* f0 = ((u8**)(gBattle + 0xA00))[0];
        u8* f1 = ((u8**)(gBattle + 0xA00))[1];
        if (f0 != NULL && f0 + 0x20 == stats) fighter = f0;
        else if (f1 != NULL && f1 + 0x20 == stats) fighter = f1;
    }
    if (fighter == NULL) return 0;
    s32 energy = *(s32*)(*(u8**)(fighter + 0x24) + 0xC);
    s32 n = 0;
    const u8* db = gGameDb->data;
    for (s32 i = 0; i < 5; i++) {
        u8* obj = ((u8**)fighter)[i];
        if (obj == NULL) continue;
        PartInstance* part = *(PartInstance**)obj;
        if (part == NULL) continue;
        PartDef def;
        PartDbGetDef(&def, gGameDb, part->defIndex);
        u8** slot = (u8**)(obj + 0x28);
        if (FighterStatsHasStatus((PartInstance**)slot, 0xD)) continue;
        if (fn_80212E28(slot)) continue;
        for (s32 k = 0; k < 2; k++) {
            s32 actionIdx = (k == 0) ? def.action1 : def.action2;
            const u8* row = *(const u8**)(db + 0x430) + actionIdx * 0x14;
            u32 key = *(const u32*)row;
            s32 type = *(const s32*)(row + 4);
            s32 kind = *(const s32*)(*(const u8**)(db + 0x08) + type * 0x24 + 8);
            if (!(kind != 5 || fighter[0x18] != 0)) continue;
            BOOL has14 = FALSE;
            for (s32 e = 0; e < 8; e++) {
                u8* eff = *(u8**)(stats + 0x28 + e * 4);
                if (eff != NULL && *(s32*)(eff + 8) == 0xE) { has14 = TRUE; break; }
            }
            if (has14 && kind == 3) continue;
            if (PartActionCost(part, key, fighter) > energy) continue;
            out[n].limb = obj;
            out[n].action = (s32)key;
            n++;
        }
    }
    if (n == 0) {
        out[0].action = 0x136;
        out[0].limb = *(u8**)(fighter + 0x10);
        n = 1;
    }
    return n;
}

// 0x802147E0
// The computer opponent's choice: of the usable actions whose action type has kind 0, pick one
// at random into `choice` (action, limb flag 0, limb from the owner object's +0x04); if none,
// the choice is left as (0, 0, 4).
void BattleAiPickAction(void* choice, u8* stats) {
    ActionOption opts[10];
    s32 n = BattleListActions(stats, opts);
    const u8* db = gGameDb->data;
    for (s32 i = 0; i < n; i++) {
        const u8* row = *(const u8**)(db + 0x430) + opts[i].action * 0x14;
        s32 type = *(const s32*)(row + 4);
        if (*(const s32*)(*(const u8**)(db + 0x08) + type * 0x24 + 8) != 0) {
            opts[i] = opts[n - 1];
            i--;
            n--;
        }
    }
    fn_8020F384(choice, 0, 0, 4);
    if (n > 0) {
        s32 r = RandomUpTo(n - 1);
        *(s32*)choice = opts[r].action;
        *(s32*)((u8*)choice + 8) = *(s32*)(opts[r].limb + 4);
    }
}

}  // extern "C"
