// Small battle, debug-menu and UI-state helpers from the 0x800293C8-0x8004415C range
// (battle record reset, object teardown, fighter and part status pushes, toggle state).
#include "types.h"

extern "C" {
void BattleReset(u8* b);
void FighterStatsSetOwner(u8* stats, void* owner);
void PartAnimKey(char* out, s32 limb, s32 defId);
void PartSetDefect(u8* p, s32 d);
void PartSetEnhancement(u8* p, s32 e);
char* StrCopy(char* dst, const char* src);
int SysSprintf(char* buf, const char* fmt, ...);
s32 fn_8002BFDC(u8* p, u8* vt);       // 0x8002BFDC, return unused by callers here
s32 fn_8002C040(u8* p, s32 v);        // 0x8002C040, return unused by callers here
void* fn_8003FB24(void);              // 0x8003FB24
u8* fn_801B3938(u8* a3);              // 0x801B3938
void fn_801B4308(u8* p, const char* text);
void* fn_801D40E0(s32 n);
void fn_801EBA20(void* list, void* item);
const char* fn_8020E438(s32 idx);
u8* fn_8020F3D4(u8* p, u8* arg1);     // 0x8020F3D4
void* fn_8020F564(void* mem, u8* owner);  // 0x8020F564
void operator_delete(void* p);
extern u8* gBattle;
extern u8 lbl_8035763C[];   // 0x8035763C (vtable)
extern u8 lbl_803584D4[];   // 0x803584D4 (vtable)
extern u8 lbl_80388EB0[];   // 0x80388EB0 (vtable)
extern const char lbl_80388E98[];  // 0x80388E98 (string)
extern const char lbl_803584C0[];  // 0x803584C0 (string)
extern const char lbl_803588F8[];  // 0x803588F8 (format string)
extern u8 lbl_805FCAA8[];   // 0x805FCAA8 (float constant, read as f32)
extern u8 lbl_805FCAAC[];   // 0x805FCAAC (float constant, read as f32)
extern u8 lbl_805FCAB0[];   // 0x805FCAB0 (float constant, read as f32)
extern u8 lbl_8047C7D8[];
}

typedef s32 (*CountFn)(u8* obj);

extern "C" {

// Walks the list at +0 (flag byte) / +4 (head) and frees each node through fn_801EBA20.
// 0x801B4160
void fn_801B4160(u8* arg0) {
    while (*(u8*)arg0 != 0) {
        u8* node = *(u8**)(arg0 + 0x4);
        if (node != NULL) {
            u8* next = *(u8**)(node - 0x4);
            u8* newHead = NULL;
            if (next != NULL) {
                newHead = next + 0x8;
            }
            fn_801EBA20(*(void**)(arg0 + 0x8), node);
            *(u8**)(arg0 + 0x4) = newHead;
            if (newHead == NULL) {
                *(u8*)arg0 = 0;
            }
        } else {
            *(u8*)arg0 = 0;
        }
    }
}

// Finds the table slot for key arg1 in the two-entry id table at +0x34 (ids at +0x18, u16)
// and returns the entry at index +0x38 of that slot's array, if in range.
// 0x801AC460
void* fn_801AC460(u8* arg0, s32 arg1) {
    u8* table = *(u8**)(arg0 + 0x34);
    s32 key = arg1;
    void* slot = NULL;
    s32 i;
    u8* p;
    for (i = 0, p = table; i < 2; i++, p += 2) {
        if (key == (s32)*(u16*)(p + 0x18)) {
            slot = *(void**)(table + i * 4 + 0x10);
            break;
        }
    }
    if (slot == NULL) {
        key = 0;
    }
    s32 idx = *(s32*)(arg0 + 0x38);
    void* entries = NULL;
    for (i = 0, p = table; i < 2; i++, p += 2) {
        if (key == (s32)*(u16*)(p + 0x18)) {
            entries = *(void**)(table + i * 4 + 0x10);
            break;
        }
    }
    if (entries != NULL && idx < *(s32*)(table + 0x1C) && idx >= 0) {
        return *(void**)((u8*)entries + idx * 4);
    }
    return NULL;
}

// Resets the object at arg0 (vtable, timers, 0x1C..0x30 floats) and registers its slots.
// 0x8020F498
void fn_8020F498(u8* arg0, s32 arg1) {
    *(void**)arg0 = lbl_80388EB0;
    *(u8*)(arg0 + 0x44) = 0;
    *(u8*)(arg0 + 0x45) = 0;
    *(u8*)(arg0 + 0x46) = 0;
    StrCopy((char*)(arg0 + 0x4), lbl_80388E98);
}

// Initialises a battle object: clears its fields, sets 5.0 / 0.5 timers and
// clears one s32 per entry in the count reported by the vtable.
// 0x800293C8
void fn_800293C8(u8* arg0) {
    s32 i;
    s32 off;
    *(f32*)(arg0 + 0x4) = *(f32*)lbl_805FCAA8;
    *(void**)arg0 = lbl_8035763C;
    *(s32*)(arg0 + 0x8) = 0;
    *(f32*)(arg0 + 0xC) = *(f32*)lbl_805FCAA8;
    *(s32*)(arg0 + 0x18) = 0;
    *(f32*)(arg0 + 0x34) = *(f32*)lbl_805FCAA8;
    *(s32*)(arg0 + 0xB4) = 0;
    fn_8002BFDC(arg0 + 0xB8, lbl_8035763C);
    *(s32*)(arg0 + 0x10C) = 0;
    *(s32*)(arg0 + 0x110) = 0;
    *(s32*)(arg0 + 0x11C) = 0;
    *(s32*)(arg0 + 0x124) = 0;
    *(f32*)(arg0 + 0x10) = *(f32*)lbl_805FCAAC;
    *(f32*)(arg0 + 0x14) = *(f32*)lbl_805FCAB0;
    i = 0;
    off = 0;
    while (i < (*(CountFn*)(*(u8**)arg0 + 0x14))(arg0)) {
        *(s32*)(arg0 + off + 0x104) = 0;
        i += 1;
        off += 4;
    }
    *(f32*)(arg0 + 0x1C) = *(f32*)lbl_805FCAA8;
    *(f32*)(arg0 + 0x20) = *(f32*)lbl_805FCAA8;
    *(f32*)(arg0 + 0x24) = *(f32*)lbl_805FCAA8;
    *(f32*)(arg0 + 0x28) = *(f32*)lbl_805FCAA8;
    *(f32*)(arg0 + 0x2C) = *(f32*)lbl_805FCAA8;
    *(f32*)(arg0 + 0x30) = *(f32*)lbl_805FCAA8;
}

// Teardown: clears the flag at +0x120, calls the base cleanup on +0xB8 and frees
// the object when arg1 > 0 (deleting destructor). Returns arg0.
// 0x80029660
u8* fn_80029660(u8* arg0, s32 arg1) {
    if (arg0 != NULL) {
        *(s32*)(arg0 + 0x120) = 0;
        fn_8002C040(arg0 + 0xB8, -1);
        if (arg1 > 0) {
            operator_delete(arg0);
        }
    }
    return arg0;
}

// Removes the entry matching (arg1, arg2) from the array at +0x9B4 / +0x9F0 (count at +0xA3C),
// shifting the rest down. Returns 1 when an entry was removed, 0 otherwise.
// 0x800381B8
s32 fn_800381B8(u8* arg0, s32 arg1, s32 arg2) {
    s32 count = *(s32*)(arg0 + 0xA3C);
    s32 i;
    for (i = 0; i < count; i++) {
        if (arg1 == *(s32*)(arg0 + i * 4 + 0x9B4) && arg2 == *(s32*)(arg0 + i * 4 + 0x9F0)) {
            s32 j = i;
            while (j < *(s32*)(arg0 + 0xA3C) - 1) {
                *(s32*)(arg0 + j * 4 + 0x9B4) = *(s32*)(arg0 + j * 4 + 0x9B8);
                *(s32*)(arg0 + j * 4 + 0x9F0) = *(s32*)(arg0 + j * 4 + 0x9F4);
                *(u8*)(arg0 + j + 0xA2C) = *(u8*)(arg0 + j + 0xA2D);
                j += 1;
            }
            *(s32*)(arg0 + 0xA3C) = *(s32*)(arg0 + 0xA3C) - 1;
            return 1;
        }
    }
    return 0;
}

// Debug menu callback: initialises the object, allocates a 0x9C-byte helper
// and stores it at +0x4C. Returns arg0.
// 0x8003F91C
u8* fn_8003F91C(u8* arg0, u8* arg1) {
    u8* var_r3;
    fn_8020F3D4(arg0, arg1);
    *(void**)(arg0 + 0x4C) = NULL;
    *(void**)arg0 = lbl_803584D4;
    *(u8*)(arg0 + 0x50) = 0;
    var_r3 = (u8*)fn_801D40E0(0x9C);
    if (var_r3 != NULL) {
        var_r3 = (u8*)fn_8003FB24();
    }
    *(u8**)(arg0 + 0x4C) = var_r3;
    *(u8*)(arg0 + 0x5) = 1;
    StrCopy((char*)(arg0 + 0xC), lbl_803584C0);
    return arg0;
}

// Pushes the current owner's stat text to the record's text field via fn_801B4308.
// 0x80043400
void fn_80043400(u8* arg0) {
    u8* temp_r31 = *(u8**)(arg0 + 0xD4);
    s32 var_r30 = *(s32*)(temp_r31 + 0xC);
    s32 side;
    s32 slot;
    u8* fighters;
    u8* entry;
    char sp8[0x20];
    char sp28[0x80];

    if (fn_801B3938(*(u8**)(*(u8**)temp_r31 + 0x260)) != arg0) {
        side = *(s32*)(temp_r31 + 0x4);
        slot = *(s32*)(temp_r31 + 0x8);
        fighters = *(u8**)(gBattle + side * 4 + 0xA00);
        entry = *(u8**)(fighters + slot * 4);
        if (entry != NULL) {
            u8* inner = *(u8**)entry;
            var_r30 = *(s32*)inner;
            *(s32*)(temp_r31 + 0xC) = var_r30;
        }
    }
    PartAnimKey(sp8, *(s32*)(temp_r31 + 0x8), var_r30);
    SysSprintf(sp28, lbl_803588F8, fn_8020E438(var_r30), sp8);
    fn_801B4308(arg0, sp28);
}

// Gives both fighters a 0x4C-byte owner record and clears the Hit/charge counter of
// every battle entry whose state is 2; then resets the battle and the callback list.
// 0x80043824
void fn_80043824(u8* arg0) {
    s32 i;
    s32 off;
    s32 n;
    s32 k;
    s32 off2;

    for (i = 0, off = 0; i < 2; i++, off += 4) {
        u8* fighter = *(u8**)(gBattle + off + 0xA00);
        void* owner = fn_801D40E0(0x4C);
        if (owner != NULL) {
            owner = fn_8020F564(owner, fighter);
        }
        FighterStatsSetOwner(fighter + 0x20, owner);
    }
    n = *(s32*)(gBattle + 0xA30);
    off2 = 0;
    for (k = 0; k < n; k++, off2 += 4) {
        u8* ent;
        if (k >= *(s32*)(gBattle + 0xA30)) {
            ent = NULL;
        } else {
            ent = *(u8**)(gBattle + off2 + 0xA1C);
        }
        if (*(s32*)(ent + 0x8) == 2) {
            *(u8*)(ent + 0xC) = 0;
        }
    }
    BattleReset(gBattle);
    fn_801B4160(lbl_8047C7D8);
}

// Applies the record's pending enhancement (kind 1, +0x10) or defect (kind -1, +0x14)
// to the fighter part at the record's slot, then resets the battle.
// 0x80043CF8
void fn_80043CF8(u8* arg0) {
    u8* rec = *(u8**)(arg0 + 0xD4);
    s32 side = *(s32*)(rec + 0x4);
    s32 slot = *(s32*)(rec + 0x8);
    u8* fighters = *(u8**)(gBattle + side * 4 + 0xA00);
    u8* entry = *(u8**)(fighters + slot * 4);
    if (entry != NULL) {
        s32 kind = *(s32*)(rec + 0x18);
        u8* part = *(u8**)entry;
        s32 v;
        if (kind == 1 && (v = *(s32*)(rec + 0x10)) != -1) {
            PartSetEnhancement(part, v);
        } else if (kind == -1) {
            v = *(s32*)(rec + 0x14);
            if (v != -1) {
                PartSetDefect(part, v);
            }
        }
    }
    BattleReset(gBattle);
}

// Decrements the counter at +0x4 of the record, wrapping to 0x10 below zero.
// 0x8004405C
void fn_8004405C(u8* arg0) {
    u8* rec = *(u8**)(arg0 + 0xD4);
    s32 v = *(s32*)(rec + 0x4) - 1;
    if (v < 0) {
        v = 0x10;
    }
    *(s32*)(rec + 0x4) = v;
}

// Three-state toggle on the record's flag bytes +0x4 / +0x5. alt != 0 is the entry at
// 0x8004415C; alt == 0 is the second entry at 0x8004415C + 8 (not defined separately here).
static void ToggleFlagsStep(u8* rec, s32 alt) {
    if (alt != 0) {
        if (*(u8*)(rec + 0x4) == 0) {
            *(u8*)(rec + 0x4) = 1;
            *(u8*)(rec + 0x5) = 0;
            return;
        }
        if (*(u8*)(rec + 0x5) == 0) {
            *(u8*)(rec + 0x5) = 1;
            return;
        }
        *(u8*)(rec + 0x5) = 0;
        return;
    }
    if (*(u8*)(rec + 0x4) == 0) {
        *(u8*)(rec + 0x4) = 1;
        *(u8*)(rec + 0x5) = 1;
        return;
    }
    if (*(u8*)(rec + 0x5) == 0) {
        *(u8*)(rec + 0x4) = 0;
        return;
    }
    *(u8*)(rec + 0x5) = 0;
}

// Toggle step for the record at +0xD4 (first entry point).
// 0x8004415C
void fn_8004415C(u8* arg0) {
    ToggleFlagsStep(*(u8**)(arg0 + 0xD4), 1);
}

}  // extern "C"
