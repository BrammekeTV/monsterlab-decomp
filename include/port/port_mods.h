// PC port: mods (port_mods.cpp, docs/modding.md). A mod is a folder mods/<name>/ next to the exe:
//   mods/<name>/files/<disc path>    replaces that file of the disc (for a disc image and an extracted folder alike)
//   mods/<name>/textures/..._<16 hex digits>.png   replaces the game texture with that content hash (or .dds)
//   mods/<name>/data/strings.json                   changes entries of the game's text table (port_moddata.cpp)
//   mods/<name>/textures/<texture name>.png         replaces the game texture of that name in the game's data (wins over a hash)
// The list and its order are kept in monsterlab.ini, [mods]: one "name = 1" (on) or "name = 0" (off)
// line per mod. Where two mods hold the same file, the one later in the list wins.
// A mod can describe itself in mods/<name>/mod.ini (name, description, author, version, priority, enabled).
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
    // From the mod's own mod.ini (all optional); read by the scan.
    char title[64];           // name =        shown instead of the folder's name
    char about[200];          // description =
    char author[48];
    char version[24];
    int  priority;            // where a mod new to monsterlab.ini goes in the list: a higher number later (wins)
} PortMod;

extern PortMod gPortMods[PORT_MAX_MODS];   // in load order
extern int     gPortModCount;

const char* PortModsDir(void);                 // <exe folder>\mods, or --mods DIR
void PortModsSetDir(const char* dir);
// Adds a mod to the end of the list (monsterlab.ini, or a folder found by the scan); the entry, or NULL when full.
PortMod* PortModsAdd(const char* name, BOOL enabled);
// Looks at the mods folder: marks the listed mods that are there and reads their mod.ini. A folder that is
// not listed yet goes in front of the first mod of a higher priority (else at the end), switched on unless
// its mod.ini says "enabled = 0". The order and switches of listed mods are the player's and stay.
void PortModsScan(void);
// Moves the mod at index one place up (dir -1) or down (dir 1) in the load order; FALSE at the end of the list.
BOOL PortModsMove(int index, int dir);
// The host path of the file that replaces this disc file, from the last enabled mod that has it.
BOOL PortModsFindFile(const char* discPath, char* host, u32 n, const char** modName);
// Logs "mod: <mod> overrides <disc path>" the first time the game opens that file from that mod (TRUE then).
BOOL PortModsNoteOverride(const char* modName, const char* discPath);
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
// A replacement picture: RGBA8 from a PNG (dxgi 0, with the smaller levels made here) or the compressed
// blocks of a DDS, level after level. `data` is from malloc.
#define PORT_DXGI_BC1 71
#define PORT_DXGI_BC3 77
#define PORT_DXGI_BC7 98
typedef struct PortModImage {
    u32 w, h, levels;
    u32 dxgi;                 // 0 (RGBA8) or PORT_DXGI_*
    u8* data;
} PortModImage;
// The replacement for that texture. `name` is the texture's name in the game's data (or NULL): a file called
// <name>.png or <name>.dds wins over one found by hash. The files are read by worker threads, so the first
// answer is usually PORT_MODTEX_WAIT: ask again at a later picture. With PORT_MODTEX_READY, *image belongs
// to the cache and is good until the next call of this function or of PortModsRefreshTextures.
#define PORT_MODTEX_NONE  0    // no mod replaces it (or the file cannot be read)
#define PORT_MODTEX_WAIT  1    // being read
#define PORT_MODTEX_READY 2
int  PortModsTexture(u64 hash, const char* name, const PortModImage** image);
// The bytes of mip level `level` of such a picture, and the bytes of one row of it (of blocks) in *pitch.
u32  PortModLevelBytes(u32 dxgi, u32 w, u32 h, u32 level, u32* pitch);
// Dolphin texture packs: a file called tex1_<W>x<H>[_m]_<hash>[_<palette hash>]_<format>.png or .dds replaces the
// texture Dolphin would give that name. The renderer calls this for each texture it decodes, after
// PortModsTextureHash (`content`); it does nothing unless a mod has such files. `data`: level 0 as the game
// holds it (`bytes` long); `palette`: the whole palette of a paletted format.
void PortModsDolphinName(u64 content, const u8* data, u32 bytes, const u8* palette, u32 w, u32 h, u32 fmt);
u64  PortXxh64(const u8* p, size_t n);        // XXH64, seed 0 (the hash in those names)
// ---- DDS (port_dds.cpp): BC1, BC3 or BC7 with mip levels ----
BOOL PortDdsRead(const char* path, PortModImage* out, const char** why);   // FALSE and the reason
// The name the game's data gives the texture whose pixel data is at `pixels`, or NULL (not every file is loaded by the port's loader yet).
const char* PortModsTextureName(const void* pixels);
// Texture dumping (--dump-textures DIR, or Settings > Mods > Dump textures): writes the decoded texture as
// DIR\tex_<w>x<h>_f<format>_<hash>.png, once per hash; a file that is there already is left as it is.
void PortModsDumpTexture(u64 hash, const char* name, const u32* rgba, u32 w, u32 h, u32 fmt);
const char* PortModsDumpDir(void);             // where textures are dumped now, NULL when dumping is off
const char* PortModsDumpFolder(void);          // the folder of the settings (the default when none is set)
u32  PortModsDumpCount(void);                  // PNGs written in this run (to the present folder)
u32  PortModsDumpPresent(void);                // textures drawn whose PNG was there already
u32  PortModsDumpFailed(void);                 // PNGs that could not be written

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
