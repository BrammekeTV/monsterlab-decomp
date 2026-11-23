// PC port: mods (port_mods.cpp, docs/modding.md). A mod is a folder mods/<name>/ next to the exe:
//   mods/<name>/files/<disc path>    replaces that file of the disc (for a disc image and an extracted folder alike)
// The list and its order are kept in monsterlab.ini, [mods]: one "name = 1" (on) or "name = 0" (off)
// line per mod. Where two mods hold the same file, the one later in the list wins.
#ifndef PORT_MODS_H
#define PORT_MODS_H

#include "types.h"

#define PORT_MAX_MODS 64

typedef struct PortMod {
    char name[64];            // the folder's name
    BOOL enabled;
    BOOL present;             // the folder is there (a mod listed in the ini may be gone)
} PortMod;

extern PortMod gPortMods[PORT_MAX_MODS];   // in load order
extern int     gPortModCount;

const char* PortModsDir(void);                 // <exe folder>\mods, or --mods DIR
void PortModsSetDir(const char* dir);
// Adds a mod to the end of the list (monsterlab.ini, or a folder found by the scan); the entry, or NULL when full.
PortMod* PortModsAdd(const char* name, BOOL enabled);
// Looks at the mods folder: marks the listed mods that are there, appends new folders (switched on).
void PortModsScan(void);
// The host path of the file that replaces this disc file, from the last enabled mod that has it.
BOOL PortModsFindFile(const char* discPath, char* host, u32 n, const char** modName);
// Logs every file each enabled mod replaces (--verbose).
void PortModsLogOverrides(void);
int  PortSelfTestMods(void);

#endif
