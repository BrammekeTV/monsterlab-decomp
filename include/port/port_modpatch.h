// PC port: mods that add to the game's data (port_modpatch.cpp, docs/modding.md). mods/<name>/patches/*.json
// changes or appends rows of the game database and adds monster parts; the tables are patched in memory after
// global_binary.sir is loaded, so a mod holds only its own entries and no game data.
//
// A modded part is a new row of the part table, behind the game's 186. A save holds a part as its row number
// (one byte), so a part keeps its row for good: <save folder>/save.modparts lists "row|vanilla row|mod:id".
// Load order, new mods and removed mods never move a part. While its mod is off, the row is a copy of the
// vanilla part it was made from, so the player keeps the part and gets the mod's one back with the mod.
#ifndef PORT_MODPATCH_H
#define PORT_MODPATCH_H

#include "types.h"
#include "port/sir_loader.h"

#define PORT_VANILLA_PARTS 186
#define PORT_MAX_PART_ROWS 255                 // row 0xFF means "none" in a save

typedef struct PortModPart {
    const char* key;          // "<mod folder>:<id>"
    s32 defIndex;             // its row in the part table
    s32 copiedFrom;           // the vanilla row it was made from (shown while the mod is off)
    const char* modDir;       // the mod's folder, for its own files
} PortModPart;

// ---- for code that gives a modded part more than a table row (model, icon, unlocking) ----
// The parts of the enabled mods. The list is complete once global_binary.sir has been converted (or, after
// loading a savestate, from the first frame on).
int  PortModPartCount(void);
const PortModPart* PortModPartAt(int i);
s32  PortModPartIndex(const char* key);                  // the row, or -1 when no enabled mod has that part
const char* PortModPartData(int i, const char* name);    // an entry of the part's "data" object, or NULL
const char* PortModPartName(int i);                      // the part's first name ("names" of its patch), else its key
const char* PortModPartsSidecar(void);                   // <save folder>/save.modparts

// ---- giving the parts to the player (port_modgrant.cpp) ----
// Once a profile has a monster of its own (the first one is made in the Lightning Tower), every part of the enabled
// mods that this profile was not given yet goes into its inventory, a notice names them, and save.modparts records
// "given|<slot>|<scientist name>|<mod:id>" so a part is never given twice (also not after it was sold).
// [modparts] give = 0 in monsterlab.ini turns this off; PORT_MODPART_GIVE=1 (tests) gives them without a monster.
void PortModGrantFrame(void);
int  PortSelfTestModGrant(void);

// ---- hooks ----
void PortModPatchApply(u8* globalHdr);                   // global_binary.sir was converted
void PortModPatchFrame(void);                            // once per frame: applies the patches after a savestate
extern "C" s32  PortModTextId(u32 hash);                 // a text key the game's table does not have: -1, or a mod's name
extern "C" void PortModPartEnsureRow(s32 def);           // a save holds row `def`: make sure the table has it

// The work itself, on any game database block (`db`: the table descriptors) and string table; `sidecar` is
// the save.modparts path. For the self-test.
void PortModPatchRun(u8* db, SirStringTable* strings, const char* sidecar);
int  PortSelfTestModPatch(void);

#endif
