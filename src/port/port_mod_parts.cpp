// Mods that add a monster part (mods\<mod>\parts\*.part, made by tools/mods/<mod>/build_mod.py; docs/modding.md).
//
// A new part is a new row of the part table whose model is a "template" part of the player's own disc with
// the mod's geometry and pictures written over it in memory:
//   - the row is a copy of the template's row with its own model number (so its model key is new) and names;
//   - the level's part list gets two entries for the new key (one per fighter) that name the template's zones;
//   - when a template zone is in memory and the new part is the one in use there, the mod's blocks are
//     written over the zone and the zone's name hash becomes the new key's; when the template itself is in
//     use again, the bytes that were there are put back. A zone is read from disc only when another one took
//     its place, so this is checked every frame and not only at the read;
//   - the names get new text ids behind the game's string table;
//   - with PORT_MODPART_GIVE=1 (tests) a game that has no such part in its inventory gets one. Not by default:
//     the row number is not save-safe yet, so nothing here may reach a save.
// Nothing here runs unless an enabled mod has a parts folder.
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "port/port.h"
#include "port/port_mods.h"
#include "port/sir_loader.h"
#include "game/part.h"

struct StringTable;
extern StringTable* gStringTable;                                  // 0x805FB910: SirStringTable (header slot 0xDA8 of global_binary.sir)

extern "C" {
u8*  fn_801F06FC(void);                                            // 0x801F06FC: the world's parameter block; **(+8) is the level header
u32  StringHash(const char* s);                                    // 0x80347E68 table: CRC-32 of the upper-cased name
void AddPartToPlayerInventory(u32 def, s32 tier, s32 quality);     // 0x802215FC
extern u8* gSaveSystem;                                            // 0x805FBBE0
}

namespace {

struct Block { u32 kind, off, size; const u8* data; };
struct Pack {                       // one .part file (host memory: read again by every run)
    char key[32];
    char mod[64];
    u32  templateRow, modelNumber, templateHash, zone[2], zoneSize, defIdOff;
    char name[PART_TIERS][48];
    std::vector<Block> blocks;
    u32  hash;                      // of "<key>_geoA": what the part list and the zone are asked for
    u32  nameHash[PART_TIERS];
    u8*  backup[2];                 // the template's bytes under the blocks, per fighter zone
};
std::vector<Pack> sPacks;
BOOL sScanned;

struct ListEntry { s32 users; u32 hash; u32 obj; s32 zone; };      // an entry of the level's part list

}  // namespace

// What a savestate must hold: the tables the game now points at lie in PortHostAlloc memory, these say so.
PORT_SAVED_BEGIN
#define MAX_PACKS 8
static s32        sRow[MAX_PACKS];            // the new row's index in the part table, 0 = not added yet
static s32        sTextBase[MAX_PACKS];       // text id of the first tier name, 0 = none
static ListEntry* sEntries[MAX_PACKS];        // the pack's two list entries
static u8**       sListCopy;                  // the level's list with our entries behind it
static u32        sListCap;
static u8*        sZoneAt[MAX_PACKS][2];      // where the template zone lies while it is in memory
static u8         sZoneIsMod[MAX_PACKS][2];   // the mod's blocks are written over it
PORT_SAVED_END

namespace {

u32 Rd32(const u8* p) { u32 v; memcpy(&v, p, 4); return v; }

BOOL LoadPack(const char* mod, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return FALSE;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    u8* d = (u8*)malloc(n > 0 ? n : 1);                            // lives as long as the game: the blocks point into it
    BOOL ok = d && n >= 0x104 && fread(d, 1, n, f) == (size_t)n && !memcmp(d, "MLP1", 4) && Rd32(d + 4) == 1;
    fclose(f);
    Pack p = {};
    if (ok) {
        memcpy(p.key, d + 8, 31);
        snprintf(p.mod, sizeof(p.mod), "%s", mod);
        p.templateRow = Rd32(d + 0x28); p.modelNumber = Rd32(d + 0x2C); p.templateHash = Rd32(d + 0x30);
        p.zone[0] = Rd32(d + 0x34); p.zone[1] = Rd32(d + 0x38); p.zoneSize = Rd32(d + 0x3C); p.defIdOff = Rd32(d + 0x40);
        for (int i = 0; i < PART_TIERS; i++) memcpy(p.name[i], d + 0x44 + i * 48, 47);
        u32 count = Rd32(d + 0x104), o = 0x108;
        for (u32 i = 0; ok && i < count; i++) {
            if (o + 12 > (u32)n) { ok = FALSE; break; }
            Block b = { Rd32(d + o), Rd32(d + o + 4), Rd32(d + o + 8), d + o + 12 };
            o += 12 + ((b.size + 3) & ~3u);
            if (o > (u32)n || b.kind > 1) { ok = FALSE; break; }
            if (b.kind == 0 && (b.off > p.zoneSize || b.size > p.zoneSize - b.off)) ok = FALSE;
            for (u32 k = 0; b.kind == 1 && k + 8 <= b.size; k += 8)
                if (Rd32(b.data + k) + 4 > p.zoneSize || Rd32(b.data + k + 4) >= p.zoneSize) ok = FALSE;
            p.blocks.push_back(b);
        }
        if (p.defIdOff + 4 > p.zoneSize) ok = FALSE;
    }
    if (!ok) { fprintf(stderr, "[port] mods: %s: %s is not a part file this version reads; ignored\n", mod, path); free(d); return FALSE; }
    char name[64];
    snprintf(name, sizeof(name), "%s_geoA", p.key);
    p.hash = StringHash(name);
    for (int i = 0; i < PART_TIERS; i++) { snprintf(name, sizeof(name), "MOD_%s_TIER%d", p.key, i + 1); p.nameHash[i] = StringHash(name); }
    sPacks.push_back(p);
    PortLog("mods: %s adds the part %s (\"%s\"; drawn over the zones %u and %u of shared_parts.sir)", mod, p.key, p.name[0], p.zone[0], p.zone[1]);
    return TRUE;
}

void Scan(void) {
    if (sScanned) return;
    sScanned = TRUE;
    if (gPortOptions.noMods) return;
    for (int i = 0; i < gPortModCount && sPacks.size() < MAX_PACKS; i++) {
        const PortMod* m = &gPortMods[i];
        if (!m->enabled || !m->present) continue;
        char pattern[MAX_PATH + 96];
        if ((u32)snprintf(pattern, sizeof(pattern), "%s\\%s\\parts\\*.part", PortModsDir(), m->name) >= sizeof(pattern)) continue;
        WIN32_FIND_DATAA fd;
        HANDLE h = FindFirstFileA(pattern, &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            char path[MAX_PATH + 96];
            if ((u32)snprintf(path, sizeof(path), "%s\\%s\\parts\\%s", PortModsDir(), m->name, fd.cFileName) < sizeof(path) && sPacks.size() < MAX_PACKS)
                LoadPack(m->name, path);
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
}

// The bytes a block covers: kind 0 its own range, kind 1 the words it sets.
template <class F> void EachRange(const Block& b, F f) {
    if (b.kind == 0) f(b.off, b.size, b.data);
    else for (u32 k = 0; k + 8 <= b.size; k += 8) f(Rd32(b.data + k), 4u, b.data + k + 4);
}

u32 BackupSize(const Pack& p) {
    u32 n = 4;
    for (const Block& b : p.blocks) EachRange(b, [&](u32, u32 size, const u8*) { n += size; });
    return n;
}

// Keeps the template's bytes (zone as read and converted) so they can be put back.
void TakeBackup(Pack& p, int k, const u8* zone) {
    if (!p.backup[k]) p.backup[k] = (u8*)malloc(BackupSize(p));
    u8* out = p.backup[k];
    if (!out) return;
    memcpy(out, zone + p.defIdOff, 4); out += 4;
    for (const Block& b : p.blocks) EachRange(b, [&](u32 off, u32 size, const u8*) { memcpy(out, zone + off, size); out += size; });
}

// PORT_MODPART_SHOW=1 (tests): the new part's model is drawn wherever its template is in use, so a savestate
// of a fight with the template shows it without a visit to the lab. The zone keeps the template's name.
BOOL TestShow(void) {
    static int on = -1;
    if (on < 0) on = getenv("PORT_MODPART_SHOW") != NULL;
    return on;
}

void SetIdentity(Pack& p, int pi, int k, BOOL mod) {
    u8* zone = sZoneAt[pi][k];
    if (!zone || sZoneIsMod[pi][k] == (u8)mod) return;
    if (mod) {
        if (Rd32(zone + p.defIdOff) != p.templateHash) {            // another disc version, or not the zone we think
            PortLog("mods: %s: zone %u is not the template of %s (name hash %08x); the part is not drawn", p.mod, p.zone[k], p.key, Rd32(zone + p.defIdOff));
            sZoneAt[pi][k] = NULL;
            return;
        }
        TakeBackup(p, k, zone);
        for (const Block& b : p.blocks) {
            if (b.kind == 0) memcpy(zone + b.off, b.data, b.size);
            else for (u32 i = 0; i + 8 <= b.size; i += 8) {         // an address inside the display list: big-endian, as the console has it
                u32 a = ((u32)(uintptr_t)zone + Rd32(b.data + i + 4)) & 0x3FFFFFFFu;
                u8* w = zone + Rd32(b.data + i);
                w[0] = (u8)(a >> 24); w[1] = (u8)(a >> 16); w[2] = (u8)(a >> 8); w[3] = (u8)a;
            }
        }
        if (!TestShow()) memcpy(zone + p.defIdOff, &p.hash, 4);
    } else {
        const u8* in = p.backup[k];
        if (!in) { PortLog("mods: %s: the template of %s cannot be put back in zone %u (loaded from a state); it shows the mod's model until the zone is read again", p.mod, p.key, p.zone[k]); return; }
        memcpy(zone + p.defIdOff, in, 4); in += 4;
        for (const Block& b : p.blocks) EachRange(b, [&](u32 off, u32 size, const u8*) { memcpy(zone + off, in, size); in += size; });
    }
    sZoneIsMod[pi][k] = (u8)mod;
    PortLog("mods: %s: zone %u now holds %s", p.mod, p.zone[k], mod ? p.key : "its own part");
}

// The part table (table 22 of the game database) with one more row.
void AddRow(Pack& p, int pi) {
    u8* db = gGameDb ? gGameDb->data : NULL;
    if (!db) return;
    PartDef* rows; u32 count;
    memcpy(&rows, db + 0x5B78, sizeof(rows));
    memcpy(&count, db + 0x5B7C, 4);
    if (!rows || p.templateRow >= count) return;
    PartDef* grown = (PartDef*)PortHostAlloc((count + 1) * sizeof(PartDef));
    if (!grown) return;
    memcpy(grown, rows, count * sizeof(PartDef));
    PartDef* d = &grown[count];
    *d = grown[p.templateRow];
    d->index = (s32)count;
    d->unk48[2] = (u8)p.modelNumber;
    for (int i = 0; i < PART_TIERS; i++) d->tierName[i] = p.nameHash[i];
    u32 n = count + 1;
    memcpy(db + 0x5B78, &grown, sizeof(grown));
    memcpy(db + 0x5B7C, &n, 4);
    memcpy(db + 0x5B8C, &n, 4);                                     // the table's second count
    sRow[pi] = (s32)count;
    PortLog("mods: %s: part %s is row %u of the part table (copied from row %u)", p.mod, p.key, count, p.templateRow);
}

// While a level is being read its header can name a list that is not in memory or not converted yet, so every
// pointer is checked to be readable before it is followed (the last region asked about is remembered: the
// entries lie together).
BOOL InMemory(const void* p, u32 size) {
    static uintptr_t lo, hi;
    uintptr_t a = (uintptr_t)p;
    if (!p || a + size < a) return FALSE;
    if (a >= lo && a + size <= hi) return TRUE;
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(p, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) || !mbi.Protect) return FALSE;
    lo = (uintptr_t)mbi.BaseAddress; hi = lo + mbi.RegionSize;
    return a + size <= hi;
}

// The level's part list: {users, name hash, object, zone} by pointer, at +0x44 of the level header, count at +0x48.
void AddToList(Pack& p, int pi) {
    u8* params = fn_801F06FC();
    u8** pp = params ? *(u8***)(params + 8) : NULL;
    u8* level = pp && InMemory(pp, 4) ? *pp : NULL;
    if (!level || !InMemory(level, 0x4C)) return;
    ListEntry** list = *(ListEntry***)(level + 0x44);
    s32 count = *(s32*)(level + 0x48);
    if (!list || count <= 0 || count > 4000 || !InMemory(list, (u32)count * 4)) return;
    for (s32 i = 0; i < count; i++) if (list[i] && !InMemory(list[i], sizeof(ListEntry))) return;
    if (!list[0] || list[count - 1] == NULL || (u32)list[0]->zone > 0xFFFF || (u32)list[0]->users > 0xFFFF) return;   // not converted yet
    s32 tmpl = -1, mine = -1;
    for (s32 i = 0; i < count; i++) {
        if (!list[i]) continue;
        if (tmpl < 0 && list[i]->hash == p.templateHash && list[i]->zone == (s32)p.zone[0]) tmpl = i;
        if (mine < 0 && list[i]->hash == p.hash) mine = i;
    }
    if (tmpl < 0 || tmpl + 1 >= count || !list[tmpl + 1]) return;   // a level without parts
    if (mine < 0) {
        if (!sEntries[pi]) sEntries[pi] = (ListEntry*)PortHostAlloc(2 * sizeof(ListEntry));
        if ((u8**)list != sListCopy) {
            // The level's own array is full: ours is a copy with room behind it. (A second pack adds to the same copy.)
            if (!sListCopy || sListCap < (u32)count + 2 * MAX_PACKS) {
                sListCap = (u32)count + 2 * MAX_PACKS;
                sListCopy = (u8**)PortHostAlloc(sListCap * sizeof(u8*));
            }
            if (!sListCopy || !sEntries[pi]) return;
            memcpy(sListCopy, list, count * sizeof(u8*));
            list = (ListEntry**)sListCopy;
            *(ListEntry***)(level + 0x44) = list;
        }
        if (!sEntries[pi] || (u32)count + 2 > sListCap) return;
        for (int k = 0; k < 2; k++) {
            sEntries[pi][k].users = 0;
            sEntries[pi][k].hash = p.hash;
            sEntries[pi][k].obj = list[tmpl + k]->obj;
            sEntries[pi][k].zone = list[tmpl + k]->zone;
            list[count + k] = &sEntries[pi][k];
        }
        *(s32*)(level + 0x48) = count + 2;
        PortLog("mods: %s: part %s added to the level's part list (entries %d and %d, zones %d and %d)", p.mod, p.key, count, count + 1,
                sEntries[pi][0].zone, sEntries[pi][1].zone);
        return;
    }
    // The game gives a list entry its object when the zones are set up: ours follow the template's.
    for (int k = 0; k < 2; k++) if (sEntries[pi]) sEntries[pi][k].obj = list[tmpl + k]->obj;
    // Which of the two the zone must show now.
    for (int k = 0; k < 2; k++) {
        if (!sEntries[pi] || !sZoneAt[pi][k]) continue;
        if (sEntries[pi][k].users > 0 || (TestShow() && list[tmpl + k]->users > 0)) SetIdentity(p, pi, k, TRUE);
        else if (list[tmpl + k]->users > 0) SetIdentity(p, pi, k, FALSE);
    }
}

// PORT_MODPART_GIVE=1 (tests only): a game without the part gets one, tier 1, full quality. (Parts on a monster
// stay in the inventory's list.) Off by default: the row number is not save-safe yet, and a part given here
// would be written into the player's save.
void AddToInventory(int pi) {
    static int on = -1;
    if (on < 0) on = getenv("PORT_MODPART_GIVE") != NULL;
    if (!on || !gSaveSystem || sRow[pi] <= 0) return;
    u8* inv = gSaveSystem + 0xA30;
    PartInstance** parts = (PartInstance**)(inv + 0x28);
    s32 n = *(s32*)(inv + 0x6BC);
    if (n < 0 || n >= 100) return;                                  // full: no room
    for (s32 i = 0; i < n; i++) if (parts[i] && parts[i]->defIndex == sRow[pi]) return;
    AddPartToPlayerInventory((u32)sRow[pi], 1, 100);
    PortLog("mods: part %s added to the inventory (%d parts now)", sPacks[pi].key, *(s32*)(inv + 0x6BC));
}

}  // namespace

// The names of a new part go behind the game's string table (the game reads strings[id] for id < count).
static void AddNames(int pi) {
    SirStringTable* t = (SirStringTable*)gStringTable;
    if (!t || !t->strings || !t->count) return;
    const char** grown = (const char**)PortHostAlloc((t->count + PART_TIERS) * sizeof(char*));
    char* text = (char*)PortHostAlloc(PART_TIERS * 48);
    if (!grown || !text) return;
    memcpy(grown, t->strings, t->count * sizeof(char*));
    for (int i = 0; i < PART_TIERS; i++) {
        memcpy(text + i * 48, sPacks[pi].name[i], 48);
        grown[t->count + i] = text + i * 48;
    }
    sTextBase[pi] = (s32)t->count;
    t->strings = grown;
    t->count += PART_TIERS;
}

// The game database is converted: the mods' part files are read now, so the log names them at the start.
void PortModPartsApplyData(u8* globalHdr) {
    (void)globalHdr;
    Scan();
}

// The text id of a key the game's own table does not have: -1, or a mod part's tier name.
extern "C" s32 PortModPartsTextId(u32 hash) {
    for (size_t pi = 0; pi < sPacks.size(); pi++)
        for (int i = 0; i < PART_TIERS; i++)
            if (sPacks[pi].nameHash[i] == hash && sTextBase[pi]) return sTextBase[pi] + i;
    return -1;
}

// A zone of shared_parts.sir was read to `zone` and converted.
void PortModPartsZoneRead(s32 number, u8* zone, u32 size) {
    Scan();
    for (size_t pi = 0; pi < sPacks.size(); pi++) {
        Pack& p = sPacks[pi];
        for (int k = 0; k < 2; k++) {
            if (number == (s32)p.zone[k]) {
                if (size < p.zoneSize) {                            // the blocks were checked against the size the pack names
                    PortLog("mods: %s: zone %d is %u bytes, the template of %s has %u; the part is not drawn", p.mod, number, size, p.key, p.zoneSize);
                    continue;
                }
                sZoneAt[pi][k] = zone;
                sZoneIsMod[pi][k] = 0;
                PortLog("mods: %s: zone %d read to %p (%u bytes), %s in use there", p.mod, number, zone, size,
                        sEntries[pi] && sEntries[pi][k].users > 0 ? p.key : "the template");
                if (sEntries[pi] && sEntries[pi][k].users > 0) SetIdentity(p, (int)pi, k, TRUE);
            } else if (sZoneAt[pi][k] && zone < sZoneAt[pi][k] + p.zoneSize && sZoneAt[pi][k] < zone + size) {
                sZoneAt[pi][k] = NULL;                              // another part's zone took its place
                sZoneIsMod[pi][k] = 0;
            }
        }
    }
}

// Once per frame.
void PortModPartsFrame(void) {
    Scan();
    for (size_t pi = 0; pi < sPacks.size(); pi++) {
        if (!sTextBase[pi]) AddNames((int)pi);
        if (!sTextBase[pi]) continue;
        if (sRow[pi] <= 0) AddRow(sPacks[pi], (int)pi);
        if (sRow[pi] <= 0) continue;
        AddToList(sPacks[pi], (int)pi);
        AddToInventory((int)pi);
    }
}
#endif
