// Mods that add a monster part (mods\<mod>\parts\*.part, made by tools/mods/<mod>/build_mod.py; docs/modding.md).
//
// A new part is a new row of the part table whose model is a "template" part of the player's own disc with
// the mod's geometry and pictures written over it in memory:
//   - the row and the names come from the mod's patches/*.json (the part "<mod>:head", port_modpatch.cpp): a
//     copy of the template's row with its own model number, so its model key is new;
//   - the level's part list gets two entries for the new key (one per fighter) that name the template's zones;
//   - when a template zone is in memory and the new part is the one in use there, the mod's blocks are
//     written over the zone and the zone's name hash becomes the new key's; when the template itself is in
//     use again, the bytes that were there are put back. A zone is read from disc only when another one took
//     its place, so this is checked every frame and not only at the read;
//   - the player is given the part after their first monster (port_modgrant.cpp);
//   - a part can have draws of light (glowing gas, beams): the port draws those itself, see "draws of light" below.
// Nothing here runs unless an enabled mod has a parts folder.
#ifdef _WIN32
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "port/port.h"
#include "port/port_gx.h"
#include "port/port_modpatch.h"
#include "port/port_mods.h"
#include "game/part.h"

extern "C" {
u8*  fn_801F06FC(void);                                            // 0x801F06FC: the world's parameter block; **(+8) is the level header
u32  StringHash(const char* s);                                    // 0x80347E68 table: CRC-32 of the upper-cased name
}

namespace {

struct Block { u32 kind, off, size; const u8* data; };
struct Light {                      // a block of kind 2, as it stands in the file (tools/mods/blackhole/build_mod.py)
    u32 first, end;                 // the range of the zone its draw commands lie in
    u32 flags;                      // 1: a billboard
    u32 texture;                    // where its picture lies in the zone
    u32 matrix;                     // the position matrix index its vertices name
    f32 centre[3], axis[3];         // a billboard's middle, and the direction its y follows
    f32 scroll[2];                  // turns of the picture per second
    f32 gain;
};
struct Pack {                       // one .part file (host memory: read again by every run)
    char key[32];
    char mod[64];
    u32  templateHash, zone[2], zoneSize, defIdOff;
    std::vector<Block> blocks;
    std::vector<Light> lights;
    u32  hash;                      // of "<key>_geoA": what the part list and the zone are asked for
    u8*  backup[2];                 // the template's bytes under the blocks, per fighter zone
};
std::vector<Pack> sPacks;
BOOL sScanned;

struct ListEntry { s32 users; u32 hash; u32 obj; s32 zone; };      // an entry of the level's part list

}  // namespace

// What a savestate must hold: the list the game now points at lies in PortHostAlloc memory, these say so.
PORT_SAVED_BEGIN
#define MAX_PACKS 8
static s32        sRow[MAX_PACKS];            // the part's row in the part table, 0 = it has none
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
    BOOL ok = d && n >= 0x104 && fread(d, 1, n, f) == (size_t)n && !memcmp(d, "MLP1", 4) && (Rd32(d + 4) == 1 || Rd32(d + 4) == 2);
    fclose(f);
    Pack p = {};
    if (ok) {
        memcpy(p.key, d + 8, 31);
        snprintf(p.mod, sizeof(p.mod), "%s", mod);
        p.templateHash = Rd32(d + 0x30);   // +0x28, +0x2C and the names at +0x44 are not used: patches/*.json has them
        p.zone[0] = Rd32(d + 0x34); p.zone[1] = Rd32(d + 0x38); p.zoneSize = Rd32(d + 0x3C); p.defIdOff = Rd32(d + 0x40);
        u32 count = Rd32(d + 0x104), o = 0x108;
        for (u32 i = 0; ok && i < count; i++) {
            if (o + 12 > (u32)n) { ok = FALSE; break; }
            Block b = { Rd32(d + o), Rd32(d + o + 4), Rd32(d + o + 8), d + o + 12 };
            o += 12 + ((b.size + 3) & ~3u);
            if (o > (u32)n || b.kind > 2) { ok = FALSE; break; }
            if (b.kind == 2) {
                Light l;
                if (b.size != sizeof(l)) { ok = FALSE; break; }
                memcpy(&l, b.data, sizeof(l));
                if (l.first >= l.end || l.end > p.zoneSize || l.texture >= p.zoneSize || l.matrix > 63) { ok = FALSE; break; }
                p.lights.push_back(l);
                continue;
            }
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
    sPacks.push_back(p);
    PortLog("mods: %s draws the part %s over the zones %u and %u of shared_parts.sir (%u light(s))", mod, p.key, p.zone[0], p.zone[1], (u32)sPacks.back().lights.size());
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
    else if (b.kind == 1) for (u32 k = 0; k + 8 <= b.size; k += 8) f(Rd32(b.data + k), 4u, b.data + k + 4);
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
            else if (b.kind == 1) for (u32 i = 0; i + 8 <= b.size; i += 8) {         // an address inside the display list: big-endian, as the console has it
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

// ------------------------------------------------------------------ draws of light
// A pack's blocks of kind 2 name draw commands of its display list that are light and not a surface (the black
// hole's disc, halo and beam). The game would draw them with the template's material; the port takes them out of
// the stream (PortGxFilter) and draws them itself when the scene is complete, just before the frame buffer is
// copied: with a copy of the registers as they were at the draw, changed to one stage "picture x vertex colour x 2",
// added to the frame buffer, unlit, both sides, depth tested and not written, no fog, the picture scrolling. A
// billboard light gets a position matrix that keeps the bone's place and size and faces the camera.
// The game draws a model more than once per scene (a second pass over the same list): only the first is taken, the
// others are dropped. A pass that does not have the pack's picture bound (a silhouette) gets nothing.
struct LightDraws {
    PortGx regs;
    PortGxLayout layout;
    const Light* light;
    const u8* zone;
    u32 last;                       // zone offset of the last draw taken
    u32 n;
    struct { u32 prim, count; const u8* data; } draw[24];
};
#define MAX_LIGHT_DRAWS 12
LightDraws* sLights;                // the draws waiting for the end of the scene
u32 sLightCount, sLightSerial;

void LightRegs(PortGx* r, const Light& l, u32 map) {
    const u32 sampler = (map < 4 ? 0x80 : 0xA0) + (map & 3);
    r->bp[0x00] = (r->bp[0x00] & ~0x7FC7Fu) | 1 | (1 << 4);          // 1 texture coordinate, 1 colour, 1 stage, no culling, no indirect stage
    r->bp[0x28] = (r->bp[0x28] & ~0xFFFu) | map | (1 << 6);           // stage 0: this map, coordinate 0, colour channel 0
    r->bp[0xC0] = 0xF8AF | (1 << 19) | (1 << 20);                      // colour = picture x vertex colour, x 2, clamped
    r->bp[0xC1] = 0xFFD0 | (1 << 19);                                  // alpha = the vertex's (not used)
    r->bp[0xF3] = (7 << 16) | (7 << 19);                               // no alpha test
    r->bp[0x41] = (r->bp[0x41] & 4) | 1 | (1 << 3) | (1 << 5) | (1 << 8);   // added to the picture; its alpha stays
    r->bp[0x40] = 1 | (3 << 1);                                        // depth: tested, not written
    r->bp[0x10] = 0;
    r->bp[0xF1] &= ~(7u << 21);                                        // no fog
    r->bp[sampler] = (r->bp[sampler] & ~0xFu) | 5;                     // the picture repeats
    r->xf[0x09] = 1; r->xf[0x3F] = 1;
    r->xf[0x0E] = 1; r->xf[0x10] = 1;                                  // colour and alpha: the vertex's, no lights
    r->xf[0x40] = 5 << 7;                                              // coordinate 0: the vertex's first, through texture matrix 60
    r->xf[0x50] = 60; r->xf[0x12] |= 1;                                // and post matrix 60: the scroll
    r->cp[0x30] = (r->cp[0x30] & ~(0x3Fu << 6)) | (60u << 6);
    const f32 t = (f32)PortFrameCount() / 60.0f;
    f32 su = l.scroll[0] * t, sv = l.scroll[1] * t;
    su -= floorf(su); sv -= floorf(sv);
    const f32 ident[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    memcpy(&r->xfMem[60 * 4], ident, sizeof(ident));
    f32* post = &r->xfMem[0x500 + 60 * 4];
    memcpy(post, ident, sizeof(ident));
    post[3] = su; post[7] = sv;
    if (l.flags & 1) {
        // The bone's matrix (model to view, rows of 4): the billboard keeps where `centre` lands and the bone's size.
        // Its z points from there at the eye (not along the view: at the side of the screen that is another direction,
        // and a tilted ring would look flatter or steeper there), its y is `axis` as the eye sees it, or the screen's
        // up if there is none.
        f32* m = &r->xfMem[(l.matrix & 63) * 4];
        f32 c[3], up[3], x[3], y[3], z[3];
        for (int i = 0; i < 3; i++) c[i] = m[i * 4] * l.centre[0] + m[i * 4 + 1] * l.centre[1] + m[i * 4 + 2] * l.centre[2] + m[i * 4 + 3];
        for (int i = 0; i < 3; i++) up[i] = m[i * 4] * l.axis[0] + m[i * 4 + 1] * l.axis[1] + m[i * 4 + 2] * l.axis[2];
        const f32 s = sqrtf(m[0] * m[0] + m[4] * m[4] + m[8] * m[8]);
        if (up[0] * up[0] + up[1] * up[1] + up[2] * up[2] < 0.0025f * s * s) { up[0] = 0; up[1] = 1; up[2] = 0; }
        f32 len = sqrtf(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
        if (len > 1e-6f) { z[0] = -c[0] / len; z[1] = -c[1] / len; z[2] = -c[2] / len; } else { z[0] = 0; z[1] = 0; z[2] = 1; }
        x[0] = up[1] * z[2] - up[2] * z[1]; x[1] = up[2] * z[0] - up[0] * z[2]; x[2] = up[0] * z[1] - up[1] * z[0];
        len = sqrtf(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
        if (len > 1e-4f) { x[0] /= len; x[1] /= len; x[2] /= len; } else { x[0] = 1; x[1] = 0; x[2] = 0; }   // seen along the axis
        y[0] = z[1] * x[2] - z[2] * x[1]; y[1] = z[2] * x[0] - z[0] * x[2]; y[2] = z[0] * x[1] - z[1] * x[0];
        const f32 bill[12] = {s * x[0], s * y[0], s * z[0], c[0], s * x[1], s * y[1], s * z[1], c[1], s * x[2], s * y[2], s * z[2], c[2]};
        memcpy(m, bill, sizeof(bill));
    }
    // Registers of our own: nothing the renderer kept for the game's may be taken for these, nor the other way round.
    sLightSerial++;
    r->bpGen = (r->bpGen ^ 0x80000000u) + sLightSerial; r->xfGen = (r->xfGen ^ 0x80000000u) + sLightSerial;
    r->xfMtxGen = (r->xfMtxGen ^ 0x80000000u) + sLightSerial; r->xfLightGen = (r->xfLightGen ^ 0x80000000u) + sLightSerial;
}

BOOL LightTake(const PortGx* g, u32 prim, const PortGxLayout* layout, u32 count, const u8* data) {
    for (size_t pi = 0; pi < sPacks.size(); pi++) {
        const Pack& p = sPacks[pi];
        for (int k = 0; k < 2; k++) {
            const u8* zone = sZoneAt[pi][k];
            if (!zone || !sZoneIsMod[pi][k] || data < zone || data >= zone + p.zoneSize) continue;
            const u32 off = (u32)(data - zone);
            for (const Light& l : p.lights) {
                if (off < l.first || off >= l.end) continue;
                LightDraws* d = NULL;
                for (u32 i = 0; i < sLightCount; i++) if (sLights[i].light == &l && sLights[i].zone == zone) d = &sLights[i];
                if (d) {
                    if (off <= d->last || d->n >= 24) return TRUE;  // the list again in this scene, or more strips than we keep
                } else {
                    u32 map = 8;
                    for (u32 m = 0; m < 8; m++)
                        if (PortGxHostPtr((g->bp[(m < 4 ? 0x80 : 0xA0) + (m & 3) + 0x14] & 0xFFFFFF) << 5) == zone + l.texture) { map = m; break; }
                    if (map == 8 || sLightCount >= MAX_LIGHT_DRAWS) return TRUE;
                    if (!sLights) sLights = (LightDraws*)calloc(MAX_LIGHT_DRAWS, sizeof(LightDraws));
                    if (!sLights) return TRUE;
                    d = &sLights[sLightCount++];
                    d->regs = *g; d->layout = *layout; d->light = &l; d->zone = zone; d->n = 0;
                    LightRegs(&d->regs, l, map);
                }
                d->last = off;
                d->draw[d->n].prim = prim; d->draw[d->n].count = count; d->draw[d->n].data = data;
                d->n++;
                return TRUE;
            }
        }
    }
    return FALSE;
}

void LightFlush(void) {
    const u32 n = sLightCount;
    sLightCount = 0;
    for (u32 i = 0; i < n; i++)
        for (u32 k = 0; k < sLights[i].n; k++)
            PortGxDraw(&sLights[i].regs, sLights[i].draw[k].prim, &sLights[i].layout, sLights[i].draw[k].count, sLights[i].draw[k].data);
}
const PortGxFilter kLightFilter = {LightTake, LightFlush};

// Reads the part files once; a pack with light has the port look at the draws from then on.
void Ready(void) {
    if (sScanned) return;
    Scan();
    for (const Pack& p : sPacks) if (!p.lights.empty()) PortGxSetFilter(&kLightFilter);
}

}  // namespace

// The game database is converted: the mods' part files are read now, so the log names them at the start.
void PortModPartsApplyData(u8* globalHdr) {
    (void)globalHdr;
    Ready();
}

// A zone of shared_parts.sir was read to `zone` and converted.
void PortModPartsZoneRead(s32 number, u8* zone, u32 size) {
    Ready();
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
    Ready();
    for (size_t pi = 0; pi < sPacks.size(); pi++) {
        if (sRow[pi] <= 0) {                         // known once the game database is patched
            char id[96];
            snprintf(id, sizeof(id), "%s:head", sPacks[pi].mod);
            s32 row = PortModPartIndex(id);
            if (row >= 0) sRow[pi] = row;
            else if (!TestShow()) continue;         // (a state made without the mod has no row: the test switch shows the model all the same)
        }
        if (!gGameDb || !gGameDb->data) continue;   // the row is known before the game has its database pointer
        AddToList(sPacks[pi], (int)pi);
    }
}
#endif
