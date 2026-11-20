// Script callbacks and part-action target queries (0x800743A8 - 0x800F9A48 range).
#include "game/part.h"

extern "C" {
#include "game/script_callbacks.h"
}

extern "C" {
void EventSend(void* mgr, void* target, void* from, u32 event, int a, int b, int c, int d);   // 0x801BA190
void WorldSendEvent(void* world, s32 id, int a, int b, int c);                                // 0x800D87EC
void TargetListInit(void* t);                                                                 // 0x802161DC
void TargetListDecode(void* t, const u32* mask, s32 side);                                    // 0x80216240
void* TargetListDestroy(void* t, s32 del);                                                    // 0x80216200
void fn_800625A4(u8* arg0, u8* arg1, f32 dt);                                                    // 0x800625A4
extern u8 gGame[];                                                                            // 0x803B53A0
extern u8* lbl_805FB134;                                                                      // 0x805FB134
}

extern "C++" void* gEventMgr;   // 0x805FB6C8

// Limb list filled by TargetListDecode: count sits at +0x14 (same layout as part_instance.cpp).
struct TargetListBuf {
    s32 limbs[5];
    s32 count;
};

extern "C" {

// 0x800743A8
// Calls the virtual slot 0x60 of the object in lbl_805FB134 (its secondary vtable at +4). The
// object is only used when its first word is 1; otherwise it is passed as NULL.
void OnExperimentFadeIn(void) {
    u8* obj = lbl_805FB134;
    if (obj != NULL && *(s32*)obj != 1) obj = NULL;
    (*(void (**)(u8*))(*(u8**)(obj + 4) + 0x60))(obj);
}

// 0x800F8824
// Rocket hit: notify the world, then send the impact event to the target stored at +0x24C.
void OnRocketImpact(void* a0) {
    if (a0 != NULL) {
        WorldSendEvent(gGame + 0xD5F0, 0x0E5396E4, 0, 0, 0);
        u8* target = *(u8**)((u8*)a0 + 0x24C);
        if (target != NULL) {
            EventSend(gEventMgr, target, NULL, 0x1F5C4DB7u, 0, 0, 0, 0);
        }
    }
}

// 0x8010375C
// Forwards the value at *a1 to fn_800625A4 through a stack copy (a0 and a2 pass through).
void OrientBoneToTarget(u32 a0, s32* a1, f32 a2) {
    s32 value = *a1;
    fn_800625A4((u8*)(intptr_t)a0, (u8*)&value, a2);  // lint:ok 32-bit handle
}

// Decodes the limbs hit by a part's action (action1, or action2 when useAction2 != 0).
// The mask comes from the action record at +0x08; the side is the part's slot.
static void DecodePartActionTargets(s32 defIndex, s32 useAction2, TargetListBuf* list) {
    PartDef def;
    PartDbGetDef(&def, gGameDb, defIndex);
    s32 actionId = useAction2 ? def.action2 : def.action1;
    PartAction* table = *(PartAction**)(gGameDb->data + 0x430);
    PartAction* rec = &table[actionId];
    u32 mask = rec->unk08;
    TargetListInit(list);
    TargetListDecode(list, &mask, def.slot);
}

// 0x800F2EA4
s32 PartAction1GetTarget1(s32 a0) {
    TargetListBuf list;
    if (a0 >= 0 && a0 < 0xBA) {
        DecodePartActionTargets(a0, 0, &list);
        if (list.count > 0) {
            s32 limb = list.limbs[0];
            TargetListDestroy(&list, -1);
            return limb;
        }
        TargetListDestroy(&list, -1);
    }
    return -1;
}

// 0x800F2FA0
s32 PartAction1GetTarget2(s32 a0) {
    TargetListBuf list;
    if (a0 >= 0) {
        DecodePartActionTargets(a0, 0, &list);
        if (list.count > 1) {
            s32 limb = list.limbs[1];
            TargetListDestroy(&list, -1);
            return limb;
        }
        TargetListDestroy(&list, -1);
    }
    return -1;
}

// 0x800F3AF4
s32 PartAction2GetTarget1(s32 a0) {
    TargetListBuf list;
    if (a0 >= 0 && a0 < 0xBA) {
        DecodePartActionTargets(a0, 1, &list);
        if (list.count > 0) {
            s32 limb = list.limbs[0];
            TargetListDestroy(&list, -1);
            return limb;
        }
        TargetListDestroy(&list, -1);
    }
    return -1;
}

// 0x800F3BF0
s32 PartAction2GetTarget2(s32 a0) {
    TargetListBuf list;
    if (a0 >= 0 && a0 < 0xBA) {
        DecodePartActionTargets(a0, 1, &list);
        if (list.count > 1) {
            s32 limb = list.limbs[1];
            TargetListDestroy(&list, -1);
            return limb;
        }
        TargetListDestroy(&list, -1);
    }
    return -1;
}

// PartQuery_GetDefectName (0x800F99B4) and PartQuery_GetEnhancementName (0x800F9A48) are not
// written here: they read sQueryPart, which is static in part_query.cpp.

}  // extern "C"
