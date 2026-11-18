// The player's lab inventory (save + 0xA30): up to 10 monsters, 100 parts, and 105 material
// counters (45 of kind 0, 60 of kind 1), plus the monster object basics.
// Original: 0x8021957C - 0x8021A164, 0x8021C888, 0x802209C8 (auto_03_8020F8E0_text.s)
#include <string.h>

#include "types.h"
#include "game/part.h"

extern "C" {
void* operator_new(u32 size);                                   // 0x801D40E0
void  operator_delete(void* p);                                 // 0x801D4980
void  PartInit(PartInstance* p);                                // 0x8021D320
void  PartDestroy(PartInstance* p, s32 del);                    // 0x8021D374
void  PartClear(PartInstance* p);                               // 0x8021D408
void  SaveUnlockAchievement(u8* save, s32 id);                  // 0x802233D4
void  SaveUnlockTieredAchievements(u8* save, s32 value, s32 n, const s32* ids, const s32* thresholds);  // 0x80228604
BOOL  fn_8022E170(u8* save);
}
extern u8* gSaveSystem;            // 0x805FBBE0 (+0x114 = current monster)
extern u8  gMonsterVtable[];       // 0x80389100

#define MAX_MONSTERS   10
#define MAX_PARTS      100
#define MATERIALS_A    45
#define MATERIALS_B    60
#define MATERIALS      (MATERIALS_A + MATERIALS_B)
#define MATERIAL_MAX   99

struct Material { s32 kind; s32 index; u8 count; u8 pad[3]; };   // 0xC

struct LabInventory {
    u8*           monsters[MAX_MONSTERS];   // 0x000
    PartInstance* parts[MAX_PARTS];         // 0x028
    Material      materials[MATERIALS];     // 0x1B8
    s32           unk6A8[4];                // 0x6A8
    s32           monsterCount;             // 0x6B8
    s32           partCount;                // 0x6BC
    s32           unk6C0;                   // 0x6C0
};

typedef void (*MonsterVFn)(u8*);
typedef void (*MonsterDtorFn)(u8*, s32);
#define MVT(m, off) (*(void**)(*(u8**)(m) + (off)))

static inline Material* MaterialSlot(LabInventory* inv, s32 index, s32 kind) {
    return &inv->materials[kind == 1 ? index + MATERIALS_A : index];
}

extern "C" {

// 0x802209C8
void* MemCopy(void* dst, const void* src, u32 n) { return memmove(dst, src, n); }

// 0x8021C888
PartInstance* MonsterGetPart(u8* m, s32 i) { return ((PartInstance**)(m + 4))[i]; }

// 0x8021A0E8
// Monster vtable slot 0xC.
void MonsterReset(u8* m) {
    *(u32*)(m + 0x18) = 0;
    *(u32*)(m + 0x1C) = 0;
    *(u16*)(m + 0x20) = 0;
    memset(m + 0x22, 0, 0x20);            // name
    *(s32*)(m + 0x44) = -1;
    m[0x48] = 1;
    m[0x49] = 1;
    m[0x4A] = 0;
    for (s32 i = 0; i < 5; i++) ((PartInstance**)(m + 4))[i] = NULL;
}

// 0x8021A064
u8* MonsterCtor(u8* m) {
    *(u8**)m = gMonsterVtable;
    MonsterReset(m);
    return m;
}

// 0x8021A0A8
u8* MonsterDtor(u8* m, s32 del) {
    if (m != NULL && del > 0) operator_delete(m);
    return m;
}

// 0x8021A164
// Build a monster from one of the preset specs (gGameDb->data + 0x9180, 0x48 bytes each).
void MonsterBuildPreset(u8* m, s32 index) {
    u8 spec[0x48];
    memcpy(spec, *(u8**)(gGameDb->data + 0x9180) + index * 0x48, sizeof(spec));
    ((void (*)(u8*, const u8*))MVT(m, 0x10))(m, spec);
}

// 0x8021957C
LabInventory* LabInventoryCtor(LabInventory* inv) {
    inv->monsterCount = 0;
    inv->partCount = 0;
    inv->unk6C0 = 0;
    for (s32 i = 0; i < MAX_MONSTERS; i++) {
        u8* m = (u8*)operator_new(0x4C);
        if (m != NULL) m = MonsterCtor(m);
        inv->monsters[i] = m;
    }
    for (s32 i = 0; i < MAX_PARTS; i++) {
        PartInstance* p = (PartInstance*)operator_new(0x30);
        if (p != NULL) PartInit(p);
        inv->parts[i] = p;
    }
    for (s32 i = 0; i < MATERIALS; i++) {
        inv->materials[i].count = 0;
        inv->materials[i].kind = i < MATERIALS_A ? 0 : 1;
        inv->materials[i].index = i < MATERIALS_A ? i : i - MATERIALS_A;
    }
    return inv;
}

// 0x8021984C
LabInventory* LabInventoryDtor(LabInventory* inv, s32 del) {
    if (inv == NULL) return inv;
    for (s32 i = 0; i < MAX_MONSTERS; i++)
        if (inv->monsters[i] != NULL) ((MonsterDtorFn)MVT(inv->monsters[i], 0x8))(inv->monsters[i], 1);
    for (s32 i = 0; i < MAX_PARTS; i++) PartDestroy(inv->parts[i], 1);
    if (del > 0) operator_delete(inv);
    return inv;
}

// 0x80219908
// Move a monster to the end of the list and drop it from the count (its object is reused).
void LabReleaseMonster(LabInventory* inv, u8* m) {
    for (s32 i = 0; i < inv->monsterCount; i++) {
        if (inv->monsters[i] != m) continue;
        s32 last = inv->monsterCount - 1;
        for (; i < inv->monsterCount - 1; i++) inv->monsters[i] = inv->monsters[i + 1];
        inv->monsters[last] = m;
        inv->monsterCount--;
        return;
    }
}

// 0x80219B58
s32 LabFindPart(LabInventory* inv, PartInstance* p) {
    for (s32 i = 0; i < inv->partCount; i++)
        if (inv->parts[i] == p) return i;
    return -1;
}

// 0x80219B94
// Move a part to the end of the list and drop it from the count.
void LabReleasePart(LabInventory* inv, PartInstance* p) {
    for (s32 i = 0; i < inv->partCount; i++) {
        if (inv->parts[i] != p) continue;
        if (i + 1 < inv->partCount) {
            MemCopy(&inv->parts[i], &inv->parts[i + 1], (inv->partCount - i - 1) * 4);
            inv->parts[inv->partCount - 1] = p;
        }
        inv->partCount--;
    }
}

// 0x8021998C
// Release a monster together with its five parts.
void LabReleaseMonsterAt(LabInventory* inv, s32 index) {
    for (u32 i = 0; i < 5; i++) {
        PartInstance* p = MonsterGetPart(inv->monsters[index], (s32)i);
        if (p != NULL) LabReleasePart(inv, p);
    }
    LabReleaseMonster(inv, inv->monsters[index]);
}

// 0x80219FC4
// Take the next free monster object; it becomes the current monster unless one is set.
u8* LabAllocMonster(LabInventory* inv) {
    u8* m = NULL;
    if (inv->monsterCount < MAX_MONSTERS) {
        m = inv->monsters[inv->monsterCount];
        if (m != NULL) {
            ((MonsterVFn)MVT(m, 0xC))(m);
            if (!fn_8022E170(gSaveSystem)) *(u8**)(gSaveSystem + 0x114) = m;
            if (++inv->monsterCount == MAX_MONSTERS) SaveUnlockAchievement(gSaveSystem, 0x40);   // full lab
        }
    }
    return m;
}

// 0x80219AE8
// Take the next unused part and clear it.
PartInstance* PartInventoryAlloc(LabInventory* inv) {
    PartInstance* p = NULL;
    if (inv->partCount < MAX_PARTS) {
        p = inv->parts[inv->partCount];
        PartClear(p);                         // virtual call, vtable slot 2
        inv->partCount++;
    }
    return p;
}

// 0x80219F44
void LabInventoryReset(LabInventory* inv) {
    for (s32 i = 0; i < MATERIALS; i++) inv->materials[i].count = 0;
    inv->unk6A8[3] = 0;
    inv->unk6A8[2] = 0;
    inv->unk6A8[1] = 0;
    inv->unk6A8[0] = 0;
    inv->monsterCount = 0;
    inv->partCount = 0;
    inv->unk6C0 = 0;
    if (gSaveSystem != NULL) *(u8**)(gSaveSystem + 0x114) = NULL;
}

// 0x80219C44
// Add materials (capped at 99). With `notify`, update the collection achievements: kind 0 has
// three tiers by number of different materials owned, kind 1 one achievement per group of 20.
BOOL LabAddMaterial(LabInventory* inv, s32 index, s32 kind, s32 amount, BOOL notify) {
    Material* m = MaterialSlot(inv, index, kind);
    BOOL ok;
    s32 c = amount + m->count;
    if (c < MATERIAL_MAX) {
        m->count = (u8)c;
        ok = TRUE;
    } else {
        ok = FALSE;
        m->count = MATERIAL_MAX;
    }
    if (notify) {
        if (kind == 0) {
            s32 owned = 0;
            for (s32 i = 0; i < MATERIALS_A; i++)
                if (inv->materials[i].count != 0) owned++;
            static const s32 kIds[3] = { 0x89, 0x8A, 0x8B };         // 0x803497D8
            static const s32 kThresholds[3] = { 0x7D, 0x7E, 0x7F };   // 0x803497E4
            SaveUnlockTieredAchievements(gSaveSystem, owned, 3, kIds, kThresholds);
        } else if (kind == 1) {
            s32 g1 = 0, g2 = 0, g3 = 0;
            for (s32 i = MATERIALS_A; i < MATERIALS; i++) {
                if (inv->materials[i].count == 0) continue;
                if (i >= 45 && i < 65)      g1++;
                else if (i >= 65 && i < 85) g2++;
                else if (i >= 85 && i < 105) g3++;
            }
            if (g1 >= 20) SaveUnlockAchievement(gSaveSystem, 0x82);
            if (g2 >= 20) SaveUnlockAchievement(gSaveSystem, 0x80);
            if (g3 >= 20) SaveUnlockAchievement(gSaveSystem, 0x81);
        }
    }
    return ok;
}

// 0x80219ED4
BOOL LabTakeMaterial(LabInventory* inv, s32 index, s32 kind, u32 amount) {
    Material* m = MaterialSlot(inv, index, kind);
    if (m->count >= amount) {
        m->count = (u8)(m->count - amount);
        return TRUE;
    }
    m->count = 0;
    return FALSE;
}

// 0x80219F14
BOOL LabHasMaterial(LabInventory* inv, s32 index, s32 kind, u32 amount) {
    return MaterialSlot(inv, index, kind)->count >= amount;
}

}  // extern "C"
