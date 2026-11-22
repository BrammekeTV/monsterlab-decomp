// Small functions from the miss36 draft batch: ray hit init, reactor leak queries, sound queueing,
// QuestLog / Recipes section lookups and a few tail-call wrappers.
#include "types.h"
#include "game/node_ref.h"
extern "C" {
#include "game/script_callbacks.h"
}

typedef u8* (*QueryFn)(u8* obj, u32 hash);
typedef u32 (*ObjFn)(void* obj);

// Section hashes passed to the lab-book section objects (vfunc 0x28).
static const u32 kHashQuestLog = 0x0FD3AF4D;  // "QuestLog"
static const u32 kHashRecipes = 0x9C2C1CAF;   // "Recipes"

extern "C" {
u8*  fn_8006FBDC(void);
void fn_800968C4(u8* p);                                           // 0x800968C4
s32 fn_800EF70C(u8* arg0);                                            // 0x800EF70C (stored as a callback only)
void    fn_801B6878(void* a0, u32 a1, s32 a2, s32 a3);                        // 0x801B6878
void fn_801B72A0(void* anim, void* child);                         // 0x801B72A0
void fn_801E8D74(void* out, void* list, void* owner, void* anim);  // 0x801E8D74
u32  fn_8011EFE0(u8* obj, s32 idx);                                // 0x8011EFE0
s32  fn_8011EFFC(u8* obj, s32 idx);                                // 0x8011EFFC
s32  fn_8011F080(u8* obj, s32 idx);                                // 0x8011F080
void* fn_8011F104(u8* obj, s32 idx);                               // 0x8011F104
void* fn_8011F1E0(u8* obj, s32 idx);                               // 0x8011F1E0
void* fn_8012029C(u8* obj, s32 idx);                               // 0x8012029C
void fn_80120534(u8* obj, s32 idx);                                // 0x80120534
void* memset(void* p, int c, unsigned long n);
extern u8  lbl_805FB248[];   // 0x805FB248 (lab books section table pointer)
extern s32 lbl_805FB6B8;     // 0x805FB6B8
extern f32 lbl_805FD300;     // 0x805FD300 (float 0.0)
extern f32 lbl_805FEB0C;     // 0x805FEB0C (float 0.0)
}

// Section lookup shared by the QuestLog_* and Recipes_* callbacks:
// lbl_805FB248 -> table pointer -> +8 sub-object -> vfunc 0x28(hash). NULL when a step is missing.
static u8* SectionQuery(u32 hash) {
    u8* top = *(u8**)lbl_805FB248;
    u8* obj = top ? *(u8**)(top + 0x8) : NULL;
    if (obj == NULL) {
        return NULL;
    }
    return (*(QueryFn*)(*(u8**)obj + 0x28))(obj, hash);
}

extern "C" {

// 0x80055CD8
// Zeroes a ray-hit record: float fields to 0.0, flag words to 0, and the word at +0x54 to 7.
void RayHitInit(u8* arg0) {
    *(s32*)(arg0 + 0x58) = 0;
    *(f32*)(arg0 + 0x0) = lbl_805FD300;
    *(f32*)(arg0 + 0x4) = lbl_805FD300;
    *(f32*)(arg0 + 0x8) = lbl_805FD300;
    *(f32*)(arg0 + 0xC) = lbl_805FD300;
    *(f32*)(arg0 + 0x10) = lbl_805FD300;
    *(f32*)(arg0 + 0x14) = lbl_805FD300;
    *(f32*)(arg0 + 0x18) = lbl_805FD300;
    *(f32*)(arg0 + 0x1C) = lbl_805FD300;
    *(f32*)(arg0 + 0x20) = lbl_805FD300;
    *(f32*)(arg0 + 0x24) = lbl_805FD300;
    *(f32*)(arg0 + 0x28) = lbl_805FD300;
    *(f32*)(arg0 + 0x2C) = lbl_805FD300;
    *(f32*)(arg0 + 0x30) = lbl_805FD300;
    *(f32*)(arg0 + 0x34) = lbl_805FD300;
    *(f32*)(arg0 + 0x38) = lbl_805FD300;
    *(f32*)(arg0 + 0x3C) = lbl_805FD300;
    *(s32*)(arg0 + 0x5C) = 0;
    *(s32*)(arg0 + 0x60) = 0;
    *(s32*)(arg0 + 0x64) = 0;
    *(s32*)(arg0 + 0x68) = 0;
    *(s32*)(arg0 + 0x50) = 0;
    *(s32*)(arg0 + 0x54) = 7;
}

// 0x80097488
// Current leak object of the reactor-leak manager (manager +0x1D8).
void* ReactorLeakGetCurrentLeak(void) {
    return *(void**)(fn_8006FBDC() + 0x1D8);
}

// 0x80096D3C
// Passes the manager (fn_8006FBDC result, still in r3) to fn_800968C4.
void ReactorLeakManipulateFastener(void) {
    fn_800968C4(fn_8006FBDC());
}

// 0x80098BC0
// Tail call through the object's vtable slot +0xC4.
u32 ReactorLeakManipulationDetected(void* a0) {
    return (*(ObjFn*)(*(u8**)a0 + 0xC4))(a0);
}

// 0x800EF648
// Builds a 0x80-byte sound-queue record (a1/a2 at +0/+4, callback fn_800EF70C at +0x6C) and
// hands it to fn_801E8D74 together with the owner's list at +0xFC.
void QueueSoundFX(void* a0, u32 a1, u32 a2) {
    u8 handle[8];
    u8 anim[0x80];

    memset(anim, 0, 0x64);
    *(u32*)(anim + 0x00) = a1;
    *(u32*)(anim + 0x04) = a2;
    *(s32*)(anim + 0x64) = 0;
    *(s32*)(anim + 0x68) = 0;
    *(void**)(anim + 0x6C) = (void*)fn_800EF70C;
    *(f32*)(anim + 0x70) = lbl_805FEB0C;
    *(s32*)(anim + 0x74) = 0;
    *(s16*)(anim + 0x78) = -1;
    *(s16*)(anim + 0x7A) = -1;
    *(s16*)(anim + 0x7C) = -1;
    *(s8*)(anim + 0x7E) = 0;
    *(s8*)(anim + 0x7F) = 0;
    fn_801E8D74(handle, (u8*)a0 + 0xFC, a0, anim);
}

// 0x801DDDCC
// Starts a looping sound on a valid handle; a NULL handle does nothing.
void PlayLoopingSoundFX(void* a0, u32 a1, s32 a2) {
    if (a0 == NULL) {
        return;
    }
    fn_801B6878(a0, a1, a2, 0);  // r6 is not read by the callee
}

// 0x801EE078
// Applies the mode nibble of the byte at arg0: modes 6 and 7 reload the word from lbl_805FB6B8
// through fn_801B72A0 (child = arg1 for mode 6, arg2 for mode 7) and mark bit 0x80 of the byte.
void fn_801EE078(u8* arg0, void* arg1, void* arg2) {
    s32 mode;
    s32 tmp;
    u8 b;

    // The reference word is read and written as a whole so the code does not depend on byte order.
    mode = (s32)NodeRefKind(NodeRefWord(arg0));
    if (mode == 6) {
        tmp = lbl_805FB6B8;
        fn_801B72A0(&tmp, arg1);
        *(s32*)arg0 = tmp;
    } else if (mode == 7) {
        tmp = lbl_805FB6B8;
        fn_801B72A0(&tmp, arg2);
        *(s32*)arg0 = tmp;
    } else {
        return;
    }
    b = NodeRefByte0(NodeRefWord(arg0));
    if (((b >> 4) & 8) == 0) {
        NodeRefSetByte0(arg0, (u8)(b | 0x80));
    }
}

// 0x801147F4
ScriptValue QuestLog_GetCounter(s32 a0) {
    u8* sub = SectionQuery(kHashQuestLog);
    if (sub == NULL) {
        return 0;
    }
    return (ScriptValue)fn_8011F104(sub, a0);
}

// 0x80114A04
ScriptValue QuestLog_GetSubQuestDescription(s32 a0) {
    u8* sub = SectionQuery(kHashQuestLog);
    if (sub == NULL) {
        return 0;
    }
    return (ScriptValue)fn_8011F1E0(sub, a0);
}

// 0x80114768
s32 QuestLog_HasCounter(s32 a0) {
    u8* sub = SectionQuery(kHashQuestLog);
    if (sub == NULL) {
        return 0;
    }
    return fn_8011F080(sub, a0);
}

// 0x801146DC
s32 QuestLog_HasSubQuest(s32 a0) {
    u8* sub = SectionQuery(kHashQuestLog);
    if (sub == NULL) {
        return 0;
    }
    return fn_8011EFE0(sub, a0);
}

// 0x80114880
s32 QuestLog_IsComplete(s32 a0) {
    u8* sub = SectionQuery(kHashQuestLog);
    if (sub == NULL) {
        return 0;
    }
    return fn_8011EFFC(sub, a0);
}

// 0x80114C80
ScriptValue Recipes_GetNameByIndex(s32 a0) {
    u8* sub = SectionQuery(kHashRecipes);
    if (sub == NULL) {
        return 0;
    }
    return (ScriptValue)fn_8012029C(sub, a0);
}

// 0x80114D0C
void Recipes_IncrementBodyPart(s32 a0) {
    u8* sub = SectionQuery(kHashRecipes);
    if (sub != NULL) {
        fn_80120534(sub, a0);
    }
}

}  // extern "C"
