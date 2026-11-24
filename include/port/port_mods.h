// PC port: mods (port_mods.cpp, docs/modding.md). A mod is a folder mods/<name>/ next to the exe:
//   mods/<name>/files/<disc path>    replaces that file of the disc (for a disc image and an extracted folder alike)
//   mods/<name>/textures/..._<16 hex digits>.png   replaces the game texture with that content hash
//   mods/<name>/data/strings.json                   changes entries of the game's text table (port_moddata.cpp)
//   mods/<name>/textures/<texture name>.png         replaces the game texture of that name in the game's data (wins over a hash)
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
    u32  fileCount;           // disc files under its files folder (counted by the scan)
    u32  textureCount;        // PNGs with a texture hash in their name under its textures folder
} PortMod;

extern PortMod gPortMods[PORT_MAX_MODS];   // in load order
extern int     gPortModCount;

const char* PortModsDir(void);                 // <exe folder>\mods, or --mods DIR
void PortModsSetDir(const char* dir);
// Adds a mod to the end of the list (monsterlab.ini, or a folder found by the scan); the entry, or NULL when full.
PortMod* PortModsAdd(const char* name, BOOL enabled);
// Looks at the mods folder: marks the listed mods that are there, appends new folders (switched on).
void PortModsScan(void);
// Moves the mod at index one place up (dir -1) or down (dir 1) in the load order; FALSE at the end of the list.
BOOL PortModsMove(int index, int dir);
// The host path of the file that replaces this disc file, from the last enabled mod that has it.
BOOL PortModsFindFile(const char* discPath, char* host, u32 n, const char** modName);
// Logs every file each enabled mod replaces (--verbose).
void PortModsLogOverrides(void);

// ---- textures ----
// The table of replacement PNGs, from the enabled mods in load order (the scan builds it; call it again
// after switching a mod or changing the order).
void PortModsRefreshTextures(void);
// TRUE when the renderer should hash the textures it decodes: a mod has textures, or --dump-textures.
BOOL PortModsTexturesWanted(void);
// The content hash of a game texture: size, GX format, the pixel data as the game holds it, the palette.
u64  PortModsTextureHash(const u8* data, u32 bytes, const u8* palette, u32 paletteBytes, u32 w, u32 h, u32 fmt);
// The replacement for that texture as RGBA8 from malloc (any size; the caller frees it), or NULL.
// `name` is the texture's name in the game's data (or NULL): a PNG called <name>.png wins over one found by hash.
u32* PortModsTexture(u64 hash, const char* name, u32* w, u32* h);
// The name the game's data gives the texture whose pixel data is at `pixels`, or NULL (not every file is loaded by the port's loader yet).
const char* PortModsTextureName(const void* pixels);
// --dump-textures DIR: writes the decoded texture as DIR\tex_<w>x<h>_f<format>_<hash>.png, once per hash.
void PortModsDumpTexture(u64 hash, const char* name, const u32* rgba, u32 w, u32 h, u32 fmt);
u32  PortModsDumpCount(void);

// ---- PNG (port_png.cpp): RGBA8, red in the low byte ----
BOOL PortPngWrite(const char* path, const u32* rgba, u32 w, u32 h);
u32* PortPngRead(const char* path, u32* w, u32* h, const char** why);   // malloc; NULL and the reason

// ---- data (port_moddata.cpp) ----
// Called when global_binary.sir is converted (hdr: its converted header block). With --dump-data DIR it writes the
// original text table as DIR\strings.json and exits; otherwise each enabled mod's data/strings.json changes strings.
void PortModsApplyData(u8* globalHdr);
// Applies one strings.json (text) to a table of `count` string pointers: the number of strings changed, or -1 with the
// reason (line and column) in err and nothing changed.
int  PortModsApplyStrings(const char* json, const char** strings, u32 count, char* err, u32 errSize);

int  PortSelfTestMods(void);

#endif
