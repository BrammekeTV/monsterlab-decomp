// Save progress: achievement checks after counters change, and the save <-> snapshot
// conversion (restore from / capture into the flat snapshot record used for save slots).
// Original: 0x8022C2BC - 0x8022FEC0 (auto_03_8020F8E0_text.s)
#include <string.h>

#include "types.h"
#include "game/part.h"

extern "C" {
void NoticePosted(u8* s, s32 idx);                              // 0x8022DF3C
u32  SaveHasBadge(u8* s, u32 badge);                            // 0x80223308
void SaveAddBadge(u8* s, s32 badge);                            // 0x802233D4
BOOL LabAddMaterial(u8* inv, s32 index, s32 kind, s32 amount, BOOL notify);   // 0x80219C44
BOOL LabTakeMaterial(u8* inv, s32 index, s32 kind, u32 amount); // 0x80219ED4
u8*  LabAllocMonster(u8* inv);                                  // 0x80219FC4
s32  LabFindPart(u8* inv, PartInstance* p);                     // 0x80219B48
void LabInventoryReset(u8* inv);                                // 0x80219A28
PartInstance* PartInventoryAlloc(u8* inv);                      // 0x80219AE8
void PartSetDef(PartInstance* p, s32 def);                      // 0x8021D43C
void PartSetDefect(PartInstance* p, s32 d);                     // 0x8021DA80
void PartSetEnhancement(PartInstance* p, s32 e);                // 0x8021DA88
void PartSetQuality(PartInstance* p, s32 q);                    // 0x8021DA40
void PartSetTier(PartInstance* p, u8 tier);                     // 0x8021DA68
void PartSetHealth(PartInstance* p, s32 hp);                    // 0x8021D8EC
s32  PartGetCurrentHealth(const PartInstance* p);               // 0x8021EF68
s8   PartGetQuality(const PartInstance* p);                     // 0x8021EF60
u8   PartGetTier(const PartInstance* p);                        // 0x8021EF58
void MonsterSetNameChars(u8* m, const u16* chars, u16 len);     // 0x8021C898
void SaveEndQuest(u8* s, BOOL abandon);
void SaveReset(u8* s);                                          // 0x80222300
u8*  SaveGetQuestRecord(u8* s, s32 q);                          // 0x80222F8C
void SaveSetQuestRecord(u8* s, s32 q, const u8* r);             // 0x80222FB4
void SaveSetQuestRewards(u8* s, const u8* in);                  // 0x8022309C
void SaveCopyQuestRewards(const u8* s, u8* out);                // 0x80223004
void SaveSetNextQuestLocation(u8* s, u32 text);                 // 0x80226C34
void SaveSetQuestDescription(u8* s, u32 text);                  // 0x80226C1C
void SaveRecordBestTime(u8* s, s32 i, u32 t);
void SaveRecordBestScore(u8* s, s32 i, u32 score);              // 0x80225C5C
void SaveCountLimbsLost(u8* s, s32 n);                          // 0x8022A3D8
void SaveCountLimbsBroken(u8* s, s32 n);
void SaveCountBlocks(u8* s, s32 n);
void SaveCountDodges(u8* s, s32 n);
void SaveCountBattleResult(u8* s, s32 n);                       // 0x8022B8A8
s32  SaveCountBadges(u8* s);                                    // 0x802233CC
u8*  SaveGetCurrentMonster(u8* s);                              // 0x8022E170
void fn_80227C20(u8* s, s32 idx, s32 amount);                   // 0x80227C20
void fn_802280D0(u8* s, s32 amount);                            // 0x802280D0
s32  fn_802280B0(u8* s, s32 idx);                               // 0x802280B0
s32  fn_802285FC(u8* s);                                        // 0x802285FC
void fn_80228AFC(u8* s, s32 a1);                                // 0x80228AFC
s32  fn_80229028(u8* s);                                        // 0x80229028
void fn_80229030(u8* s, s32 n);                                 // 0x80229030
s32  fn_80229544(u8* s);                                        // 0x80229544
s32  fn_802239C8(u8* s);                                        // 0x802239C8
s32  fn_80222994(u8* s);                                        // 0x80222994
void fn_8022299C(u8* s, s32 a, s32 b);                          // 0x8022299C
u8*  fn_80222704(u8* s, s32 a, s32 b);                          // 0x80222704
void fn_80222744(u8* s, s32 a, s32 b, const u8* rec);           // 0x80222744
s32  fn_80226C2C(u8* s);                                        // 0x80226C2C
s32  fn_80226C24(u8* s);                                        // 0x80226C24
u64  fn_80225664(u8* s);                                        // 0x80225664
s32  fn_8022A904(u8* s);                                        // 0x8022A904
s32  fn_8022AE68(u8* s);                                        // 0x8022AE68
s32  fn_8022B384(u8* s);                                        // 0x8022B384
s32  fn_8022B8A0(u8* s);                                        // 0x8022B8A0
s32  fn_8022BDD4(u8* s);                                        // 0x8022BDD4
s32  fn_8022C7E8(u8* s);                                        // 0x8022C7E8
void fn_8022C7F0(u8* s, s32 n);                                 // 0x8022C7F0
s32  fn_8022CD2C(u8* s);                                        // 0x8022CD2C
u32  fn_8022CD34(u8* s, s32 idx);                               // 0x8022CD34
void fn_8022CD54(u8* s, s32 idx, u32 value);                    // 0x8022CD54
void fn_8022D460(u8* s, s32 idx, s32 value);                    // 0x8022D460
u32  fn_8022D93C(u8* s, s32 idx);                               // 0x8022D93C
void fn_8022D94C(u8* s, s32 amount);                            // 0x8022D94C
s32  fn_8022DE84(u8* s);                                        // 0x8022DE84
void fn_8022E3D8(void* save, u8* name);                         // 0x8022E3D8
u8*  fn_8022E3DC(u8* s);                                        // 0x8022E3DC
}

extern u8* lbl_805FBBE0;    // 0x805FBBE0 (the save)

// Counter-driven badges: unlock `id` once the counter reaches the threshold in the tuning table
// (8-byte entries at game db + 0x99D0, float threshold at +4).
static void CheckCounterBadge(u8* s, s32 id, s32 tuning, s32 count) {
    if (SaveHasBadge(s, (u8)id)) return;
    s32 need = (s32)*(f32*)(*(u8**)(gGameDb->data + 0x99D0) + tuning * 8 + 4);
    if (count < need || id == -1) return;
    SaveAddBadge(s, id);
}

typedef void (*MonsterSetPartFn)(u8* m, s32 slot, void* part);

extern "C" {

// 0x8022C2BC
// Add to the counter at +0x9F8 and unlock any of the three badges (0x49-0x4B) it now reaches.
void fn_8022C2BC(u8* s, s32 amount) {
    static const s32 ids[3]    = { 0x49, 0x4A, 0x4B };
    static const s32 tuning[3] = { 0x7A, 0xA7, 0xA8 };
    s32 count = *(s32*)(s + 0x9F8) + amount;
    *(s32*)(s + 0x9F8) = count;
    for (s32 i = 2; i >= 0; i--) CheckCounterBadge(s, ids[i], tuning[i], count);
}

// 0x8022E658
// Consume the item picked at +0x73C (give back the one at +0x740), bump the counter at +0x9FC
// and unlock any of the four badges (0x83-0x86) it now reaches.
void fn_8022E658(u8* s) {
    static const s32 tuning[4] = { 0x8C, 0x8D, 0x8E, 0x8F };
    static const s32 ids[4]    = { 0x83, 0x84, 0x85, 0x86 };
    if (*(s32*)(s + 0x740) == -1) return;
    s32 item = *(s32*)(s + 0x73C);
    if (item == -1) return;
    const u8* rec = *(u8**)(gGameDb->data + 0x4F00) + item * 0x10;
    LabTakeMaterial(s + 0xA30, item, 0, rec[0xB]);
    LabAddMaterial(s + 0xA30, *(s32*)(s + 0x740), 0, 1, TRUE);
    s32 count = *(s32*)(s + 0x9FC) + 1;
    *(s32*)(s + 0x9FC) = count;
    for (s32 i = 3; i >= 0; i--) CheckCounterBadge(s, ids[i], tuning[i], count);
}

// 0x8022EDD8
// Rebuild the live save from a flat snapshot record: header ints, part-seen bytes, records,
// quest state, counters, then the part inventory, the monsters, and the ingredient counts.
void fn_8022EDD8(u8* snap) {
    u8* g = lbl_805FBBE0;
    SaveEndQuest(g, TRUE);
    SaveReset(g);
    LabInventoryReset(g + 0xA30);
    *(s32*)(g + 0x10) = *(s32*)(snap + 0x20);
    *(s32*)(g + 0x14) = *(s32*)(snap + 0x24);
    *(s32*)(g + 0x18) = *(s32*)(snap + 0x28);
    *(s32*)(g + 0x1C) = *(s32*)(snap + 0x2C);
    for (s32 i = 0; i < 8; i++) *(s32*)(g + 0x20 + i * 4) = *(s32*)(snap + i * 4);
    for (s32 i = 0; i < 0x9D; i++) lbl_805FBBE0[0x40 + i] = snap[0x278 + i];
    for (s32 i = 0; i < 10; i++) {
        fn_8022D460(lbl_805FBBE0, i, *(s32*)(snap + 0x318 + i * 4));
        SaveRecordBestTime(lbl_805FBBE0, i, *(u32*)(snap + 0x388 + i * 4));
        fn_80227C20(lbl_805FBBE0, i, snap[0x3B0 + i]);
    }
    for (s32 i = 0; i < 6; i++) SaveRecordBestScore(lbl_805FBBE0, i, *(u32*)(snap + 0x340 + i * 4));
    for (s32 i = 0; i < 12; i++) fn_8022CD54(lbl_805FBBE0, i, *(u32*)(snap + 0x358 + i * 4));

    g = lbl_805FBBE0;
    for (s32 i = 0; i < 4; i++) *(s32*)(g + 0xF0 + i * 4) = *(s32*)(snap + 0x40 + i * 4);
    for (s32 i = 0; i < 4; i++) *(s32*)(g + 0xE0 + i * 4) = *(s32*)(snap + 0x30 + i * 4);
    *(s32*)(lbl_805FBBE0 + 0x118) = *(s32*)(snap + 0x58);
    *(s32*)(lbl_805FBBE0 + 0x11C) = *(s32*)(snap + 0x5C);
    *(s32*)(lbl_805FBBE0 + 0x120) = *(s32*)(snap + 0x60);
    *(u8*) (lbl_805FBBE0 + 0x124) = snap[0x315];
    *(s32*)(lbl_805FBBE0 + 0x128) = *(s32*)(snap + 0x84);
    *(s32*)(lbl_805FBBE0 + 0x12C) = *(s32*)(snap + 0x64);
    *(s32*)(lbl_805FBBE0 + 0x8EC) = *(s32*)(snap + 0x68);
    *(s32*)(lbl_805FBBE0 + 0x8F0) = *(s32*)(snap + 0x6C);
    *(s32*)(lbl_805FBBE0 + 0x8F4) = *(s32*)(snap + 0x70);
    *(s32*)(lbl_805FBBE0 + 0x8F8) = *(s32*)(snap + 0x74);
    *(s32*)(lbl_805FBBE0 + 0x908) = *(s32*)(snap + 0x78);
    *(s32*)(lbl_805FBBE0 + 0x90C) = *(s32*)(snap + 0x7C);
    *(s32*)(lbl_805FBBE0 + 0x904) = *(s32*)(snap + 0x54);
    *(s32*)(lbl_805FBBE0 + 0x900) = *(s32*)(snap + 0x50);

    g = lbl_805FBBE0;
    s32 q = *(s32*)(snap + 0x88);
    if (q != -1) SaveSetQuestRecord(g, q, snap + 0x90);
    SaveSetQuestRecord(lbl_805FBBE0, 0x16, snap + 0xA8);
    SaveSetQuestRecord(lbl_805FBBE0, 0x17, snap + 0xC0);
    SaveSetQuestRecord(lbl_805FBBE0, 0x18, snap + 0xD8);
    SaveSetNextQuestLocation(lbl_805FBBE0, *(u32*)(snap + 0xF0));
    SaveSetQuestDescription(lbl_805FBBE0, *(u32*)(snap + 0xF4));
    for (s32 i = 0; i < 4; i++) {
        for (s32 j = 0; j < 3; j++) fn_80222744(lbl_805FBBE0, i, j, snap + 0x178 + i * 0x3C + j * 0x14);
    }
    SaveSetQuestRewards(lbl_805FBBE0, snap + 0xF8);
    fn_8022E3D8(lbl_805FBBE0, snap + 0x268);
    fn_8022D94C(lbl_805FBBE0, snap[0x3BA]);
    fn_80228AFC(lbl_805FBBE0, snap[0x3BB]);
    fn_80229030(lbl_805FBBE0, snap[0x3BC]);
    SaveCountLimbsLost(lbl_805FBBE0, snap[0x3BD]);
    SaveCountLimbsBroken(lbl_805FBBE0, snap[0x3BE]);
    SaveCountBlocks(lbl_805FBBE0, snap[0x3BF]);
    SaveCountDodges(lbl_805FBBE0, snap[0x3C0]);
    SaveCountBattleResult(lbl_805FBBE0, snap[0x3C1]);
    fn_8022C2BC(lbl_805FBBE0, snap[0x3C2]);
    fn_8022C7F0(lbl_805FBBE0, snap[0x3C3]);
    fn_802280D0(lbl_805FBBE0, snap[0x3C4]);

    // Lab part/monster section of the snapshot starts at +0x400.
    u8* b = snap + 0x400;
    g = lbl_805FBBE0;
    *(s32*)(g + 0x10DC) = *(s32*)(snap + 0x404);
    *(s32*)(g + 0x10D8) = *(s32*)(snap + 0x400);
    *(s32*)(g + 0x10E4) = *(s32*)(snap + 0x40C);
    *(s32*)(g + 0x10E0) = *(s32*)(snap + 0x408);
    for (s32 i = 0; i < *(s32*)(b + 0x14); i++) {
        const u8* rec = b + i * 8;
        s32 def = rec[0x1FE];
        if (def == 0xFF) def = -1;
        PartInstance* p = PartInventoryAlloc(lbl_805FBBE0 + 0xA30);
        if (p != NULL) PartSetDef(p, def);
        s32 enh = rec[0x201];
        if (enh == 0xFF) enh = -1;
        PartSetEnhancement(p, enh);
        s32 dfc = rec[0x202];
        if (dfc == 0xFF) dfc = -1;
        PartSetDefect(p, dfc);
        PartSetQuality(p, (s8)rec[0x1FF]);
        PartSetTier(p, rec[0x200]);
        PartSetHealth(p, *(s16*)(rec + 0x1FC));
    }
    *(s32*)(lbl_805FBBE0 + 0x10EC) = *(s32*)(b + 0x14);
    for (s32 i = 0; i < *(s32*)(b + 0x10); i++) {
        const u8* mr = b + i * 0x30;
        u8* m = LabAllocMonster(lbl_805FBBE0 + 0xA30);
        for (s32 k = 0; k < 5; k++) {
            u8 pi = mr[0x46 + k];
            if (pi != 0xFF) {
                (*(MonsterSetPartFn*)(*(u8**)m + 0x14))(m, k, *(void**)(lbl_805FBBE0 + 0xA58 + pi * 4));
            }
        }
        *(s32*)(m + 0x18) = *(const s32*)(mr + 0x1C);
        *(s32*)(m + 0x1C) = *(const s32*)(mr + 0x20);
        MonsterSetNameChars(m, (const u16*)(b + 0x24 + i * 0x30), *(const u16*)(mr + 0x44));
    }
    *(s32*)(lbl_805FBBE0 + 0x10E8) = *(s32*)(b + 0x10);
    s32 sel = (s8)snap[0x314];
    s32 monsters = *(s32*)(b + 0x10);
    if (monsters > sel && sel > 0) {
        *(s32*)(lbl_805FBBE0 + 0x114) = *(s32*)(lbl_805FBBE0 + 0xA30 + sel * 4);
    } else if (monsters == 0) {
        *(s32*)(lbl_805FBBE0 + 0x114) = 0;
    } else {
        *(s32*)(lbl_805FBBE0 + 0x114) = *(s32*)(lbl_805FBBE0 + 0xA30);
    }
    for (s32 i = 0; i < 0x69; i++) {
        if (i < 0x2D) LabAddMaterial(lbl_805FBBE0 + 0xA30, i, 0, b[0x51C + i], FALSE);
        else          LabAddMaterial(lbl_805FBBE0 + 0xA30, i - 0x2D, 1, b[0x51C + i], FALSE);
    }
    *(s32*)(lbl_805FBBE0 + 0x10F0) = *(s32*)(b + 0x18);
    fn_8022299C(lbl_805FBBE0, *(s32*)(snap + 0x80), *(s32*)(snap + 0x88));
}

// 0x8022F50C
// Capture the live save into a flat snapshot record (inverse of fn_8022EDD8).
void fn_8022F50C(u8* snap) {
    u8* g = lbl_805FBBE0;
    *(s32*)(snap + 0x20) = *(s32*)(g + 0x10);
    *(s32*)(snap + 0x24) = *(s32*)(g + 0x14);
    *(s32*)(snap + 0x28) = *(s32*)(g + 0x18);
    *(s32*)(snap + 0x2C) = *(s32*)(g + 0x1C);
    for (s32 i = 0; i < 8; i++) *(s32*)(snap + i * 4) = *(s32*)(g + 0x20 + i * 4);
    for (s32 i = 0; i < 0x9D; i++) snap[0x278 + i] = lbl_805FBBE0[0x40 + i];
    for (s32 i = 0; i < 10; i++) {
        *(u32*)(snap + 0x318 + i * 4) = fn_8022D93C(lbl_805FBBE0, i);
        *(u32*)(snap + 0x388 + i * 4) = *(u32*)(lbl_805FBBE0 + 0x9A8 + i * 4);
        snap[0x3B0 + i] = (u8)fn_802280B0(lbl_805FBBE0, i);
    }
    for (s32 i = 0; i < 6; i++) *(u32*)(snap + 0x340 + i * 4) = *(u32*)(lbl_805FBBE0 + 0xA04 + i * 4);
    for (s32 i = 0; i < 12; i++) *(u32*)(snap + 0x358 + i * 4) = fn_8022CD34(lbl_805FBBE0, i);

    g = lbl_805FBBE0;
    for (s32 i = 0; i < 4; i++) *(s32*)(snap + 0x40 + i * 4) = *(s32*)(g + 0xF0 + i * 4);
    for (s32 i = 0; i < 4; i++) *(s32*)(snap + 0x30 + i * 4) = *(s32*)(g + 0xE0 + i * 4);
    *(s32*)(snap + 0x58) = *(s32*)(g + 0x118);
    *(s32*)(snap + 0x5C) = fn_802239C8(lbl_805FBBE0);
    *(s32*)(snap + 0x60) = SaveCountBadges(lbl_805FBBE0);

    // Index of the current monster in the lab monster list, -1 if it is not there.
    u8* cur = SaveGetCurrentMonster(lbl_805FBBE0);
    u8* list = lbl_805FBBE0 + 0xA30;
    s32 count = *(s32*)(list + 0x6B8);
    s32 idx = -1;
    for (s32 i = 0; i < count; i++) {
        if (*(u8**)(list + i * 4) == cur) { idx = i; break; }
    }
    snap[0x314] = (u8)idx;
    snap[0x315] = lbl_805FBBE0[0x124];
    *(s32*)(snap + 0x84) = *(s32*)(lbl_805FBBE0 + 0x128);
    *(s32*)(snap + 0x64) = *(s32*)(lbl_805FBBE0 + 0x12C);
    *(s32*)(snap + 0x68) = *(s32*)(lbl_805FBBE0 + 0x8EC);
    *(s32*)(snap + 0x6C) = *(s32*)(lbl_805FBBE0 + 0x8F0);
    *(s32*)(snap + 0x70) = *(s32*)(lbl_805FBBE0 + 0x8F4);
    *(s32*)(snap + 0x74) = *(s32*)(lbl_805FBBE0 + 0x8F8);
    *(s32*)(snap + 0x78) = *(s32*)(lbl_805FBBE0 + 0x908);
    *(s32*)(snap + 0x7C) = *(s32*)(lbl_805FBBE0 + 0x90C);
    *(u64*)(snap + 0x50) = fn_80225664(lbl_805FBBE0);
    *(s32*)(snap + 0x88) = fn_80222994(lbl_805FBBE0);
    *(s32*)(snap + 0x80) = *(s32*)(lbl_805FBBE0 + 0x6A8);
    if (fn_80222994(lbl_805FBBE0) != -1) {
        u8* r = SaveGetQuestRecord(lbl_805FBBE0, fn_80222994(lbl_805FBBE0));
        if (r != NULL) {
            r = SaveGetQuestRecord(lbl_805FBBE0, fn_80222994(lbl_805FBBE0));
            memcpy(snap + 0x90, r, 0x18);
        }
    }
    memcpy(snap + 0xA8, SaveGetQuestRecord(lbl_805FBBE0, 0x16), 0x18);
    memcpy(snap + 0xC0, SaveGetQuestRecord(lbl_805FBBE0, 0x17), 0x18);
    memcpy(snap + 0xD8, SaveGetQuestRecord(lbl_805FBBE0, 0x18), 0x18);
    *(s32*)(snap + 0xF0) = fn_80226C2C(lbl_805FBBE0);
    *(s32*)(snap + 0xF4) = fn_80226C24(lbl_805FBBE0);
    for (s32 i = 0; i < 4; i++) {
        for (s32 j = 0; j < 3; j++) {
            memcpy(snap + 0x178 + i * 0x3C + j * 0x14, fn_80222704(lbl_805FBBE0, i, j), 0x14);
        }
    }
    SaveCopyQuestRewards(lbl_805FBBE0, snap + 0xF8);
    memcpy(snap + 0x268, fn_8022E3DC(lbl_805FBBE0), 0x10);
    snap[0x3BA] = (u8)fn_8022DE84(lbl_805FBBE0);
    snap[0x3BB] = (u8)fn_80229028(lbl_805FBBE0);
    snap[0x3BC] = (u8)fn_80229544(lbl_805FBBE0);
    snap[0x3BD] = (u8)fn_8022A904(lbl_805FBBE0);
    snap[0x3BE] = (u8)fn_8022AE68(lbl_805FBBE0);
    snap[0x3BF] = (u8)fn_8022B384(lbl_805FBBE0);
    snap[0x3C0] = (u8)fn_8022B8A0(lbl_805FBBE0);
    snap[0x3C1] = (u8)fn_8022BDD4(lbl_805FBBE0);
    snap[0x3C2] = (u8)fn_8022C7E8(lbl_805FBBE0);
    snap[0x3C3] = (u8)fn_8022CD2C(lbl_805FBBE0);
    snap[0x3C4] = (u8)fn_802285FC(lbl_805FBBE0);

    // Lab part/monster section at +0x400.
    u8* b = snap + 0x400;
    g = lbl_805FBBE0;
    *(s32*)(snap + 0x404) = *(s32*)(g + 0x10DC);
    *(s32*)(snap + 0x400) = *(s32*)(g + 0x10D8);
    *(s32*)(snap + 0x40C) = *(s32*)(g + 0x10E4);
    *(s32*)(snap + 0x408) = *(s32*)(g + 0x10E0);
    *(s32*)(snap + 0x410) = *(s32*)(g + 0x10E8);
    *(s32*)(snap + 0x414) = *(s32*)(lbl_805FBBE0 + 0x10EC);
    *(s32*)(snap + 0x418) = *(s32*)(lbl_805FBBE0 + 0x10F0);
    for (s32 i = 0; i < *(s32*)(b + 0x14); i++) {
        PartInstance* p = *(PartInstance**)(lbl_805FBBE0 + 0xA58 + i * 4);
        if (p == NULL) continue;
        u8* rec = b + i * 8;
        rec[0x1FE] = (u8)p->defIndex;
        *(s16*)(rec + 0x1FC) = (s16)PartGetCurrentHealth(p);
        rec[0x1FF] = (u8)PartGetQuality(p);
        rec[0x200] = PartGetTier(p);
        rec[0x201] = (u8)p->enhancement;
        rec[0x202] = (u8)p->defect;
    }
    for (s32 i = 0; i < *(s32*)(b + 0x10); i++) {
        u8* m = *(u8**)(lbl_805FBBE0 + 0xA30 + i * 4);
        if (m == NULL) continue;
        u8* mr = b + i * 0x30;
        for (s32 k = 0; k < 5; k++) {
            mr[0x46 + k] = (u8)LabFindPart(lbl_805FBBE0 + 0xA30, *(PartInstance**)(m + 4 + k * 4));
        }
        *(s32*)(mr + 0x1C) = *(s32*)(m + 0x18);
        *(s32*)(mr + 0x20) = *(s32*)(m + 0x1C);
        *(u16*)(mr + 0x44) = *(u16*)(m + 0x20);
        for (s32 j = 0; j < 16; j++) *(u16*)(mr + 0x24 + j * 2) = *(u16*)(m + 0x22 + j * 2);
    }
    // Ingredient counts: 105 bytes, stride 0xC in the save.
    for (s32 i = 0; i < 0x69; i++) b[0x51C + i] = lbl_805FBBE0[0xBF0 + i * 0xC];
}

}  // extern "C"
