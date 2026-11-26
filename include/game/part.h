#pragma once
// Monster parts: definitions (one per part type, in global_binary.sir) and instances (parts the
// player owns). See tools/parts.py to dump the definition table from the disc.
//
// Modding note: a mod part is a PartDef (plus model/texture assets). The PC port's importer
// fills PartDef records from part.json and appends them after the original 186 (rows 180-185 are
// boss parts, so the table has to be grown), so every query below works unchanged.
#include "types.h"

#define PART_TIERS 4

enum PartSlot { PART_SLOT_HEAD = 0, PART_SLOT_LEGS = 1, PART_SLOT_ARM = 2, PART_SLOT_TORSO = 4 };
enum PartDiscipline { PART_MECH = 0, PART_BIO = 1, PART_ALCHEMY = 2 };

// Action (0x14 bytes), table at gGameDb->data + 0x430, indexed by PartDef::action1/2.
struct PartAction {
    u32 key;                    // 0x00
    s32 type;                   // 0x04: index into the action type table
    u32 unk08, unk0C, unk10;
};
// Action type (0x24 bytes), table at gGameDb->data + 0x08.
struct PartActionType {
    u32 unk00[4];
    u32 nameHash;               // 0x10: text key of the action name
    u32 unk14[4];
};

// Part definition, 0x50 bytes. Table: gGameDb->data + 0x5B78 (count at +0x5B7C, 186 entries).
struct PartDef {
    s32 index;                  // 0x00
    u32 tierName[PART_TIERS];   // 0x04: text-key hashes, e.g. Chimp/Ape/Gorilla/Kong Arm
    s32 action1;                // 0x14: action id (-1 = none; then action2 is used for both)
    s32 slot;                   // 0x18: PartSlot
    s32 discipline;             // 0x1C: PartDiscipline
    s32 unk20;                  // 0x20
    s32 unk24;                  // 0x24
    s32 unk28;                  // 0x28
    s32 action2;                // 0x2C
    s32 unk30;                  // 0x30
    s32 unk34;                  // 0x34
    s32 unk38;                  // 0x38
    s32 unk3C;                  // 0x3C
    u16 unk40, unk42;           // 0x40
    u16 unk44, unk46;           // 0x44
    u8  unk48[4];               // 0x48
    u8  unk4C[4];               // 0x4C
};

// A part the player owns (inventory entry; class "XInventoryPart", 0x30 bytes).
struct PartInstance {
    s32 defIndex;               // 0x00: index into the PartDef table, -1 = empty
    s32 defect;                 // 0x04: defect id, -1 = none
    s32 enhancement;            // 0x08: enhancement id, -1 = none
    s32 uid;                    // 0x0C: unique id from a global counter
    s8  quality;                // 0x10: 0..100
    u8  tier;                   // 0x11: 1..4
    u8  tierScale[PART_TIERS];  // 0x12: per-tier percentage (health, power and madness scale)
    u8  pad16[2];
    u32 vtable;                 // 0x18: vtable (0x80389120: rtti, 0, Clear)
    s8  madnessBase;            // 0x1C
    s8  unk1D;                  // 0x1D
    u8  unk1E, unk1F;           // 0x1E
    s32 unk20;                  // 0x20
    s32 unk24;                  // 0x24
    s32 health;                 // 0x28
    u8  unk2C;                  // 0x2C: in-use flag (the original reads and writes one byte)
    u8  pad2D[3];
};

// Game database (global_binary.sir header slot 0xDC8).
struct GameDb {
    u8  pad00[0xC];
    u8* data;                   // 0x0C: +0x08 PartActionType*, +0x430 PartAction*,
                                //       +0x5B78 PartDef*, +0x5B7C count
};
extern GameDb* gGameDb;         // 0x805FBBC0

extern "C" {
void PartDbGetDef(PartDef* out, GameDb* db, s32 index);   // 0x80216838
s32  TextIdFromHash(u32 hash);                            // 0x80120D34: -1 if unknown
u8   PartGetTier(const PartInstance* p);                  // 0x8021EF58
s8   PartGetQuality(const PartInstance* p);               // 0x8021EF60
s8   PartGetMadness(const PartInstance* p);               // 0x8021EF80
s32  PartGetMaxHealth(PartInstance* p);                   // 0x8021F168: also clamps health
}
