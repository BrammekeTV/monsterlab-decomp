# Black hole head (mod, work in progress)

A new head part for the PC port: a dark core, a streaky amber accretion disc whose light bends over and
under the core, a thin photon ring, and a stabilizer on the neck that projects the hole. No bolts, no lab
parts on the hole itself.

**State: the art is made; it is not in the game yet.** The port has no way to add a part so far. What is
here, and what is missing, is listed below.

## What is here

| File | What |
|---|---|
| `make_assets.py` | draws everything from code: the 256x256 colour and gloss sheets, the 64x64 and 256x256 part icons, the model (387 vertices, 504 triangles, 7 groups) as `.obj` and `.json` |
| `sirlib.py` | small reader for the game's `.sir` containers, for the zone builder that comes next |

```
python tools/mods/blackhole/make_assets.py          # -> build/mods/BlackHoleHead (not committed)
```

Needs `numpy` and `Pillow`. Nothing of the game is read: the pictures and the model are original.
The sizes follow the game's heads: one 256x256 sheet plus a gloss sheet, icons of 64x64 and 256x256
(`ui/msa_parts/<key>` and `<key>_h`), about the triangle count of a simple head.

## How a part is stored (found for this mod)

- **Definition**: row of table 22 of the game database (`PartDef`, 0x50 bytes, 186 rows; heads are rows
  72-113, slot 0). Columns that matter: four tier names (text key hashes), actions, slot, discipline,
  the animation set (column 13; 8 = `sha`, the shared head animations) and the model number (the byte at
  `+0x4A`).
- **Model key**: `PartAnimKey` builds `p_hd_<set>_<NNN>` from those two. A new head with set `sha` and an
  unused number (this mod: 077) reuses every head animation and gets the key `p_hd_sha077`.
- **Model**: the hash of `<key>_geoA` is looked up in the level's part list (level header `+0x44`, 648
  entries of `{users, hash, object, zone}` in the village). Every part has two entries (one per fighter)
  that name two zones of `shared_parts.sir` (zones 21-452, about 660 KB each: mesh, materials, three
  textures), and one `_lr` entry for the small model on the map. A key that is not in the list falls back
  to head 001 (the game logs "The part list is missing a part it should have").
- **Icon**: `TextureLoadShared("ui/msa_parts/<key>")` and `..._h`: sections of `shared_texture.sir`, by
  name hash; a missing one falls back to `ui/msa_parts/placeholder`.
- **Owning it**: `AddPartToPlayerInventory(def, tier, quality)` adds a part to the save's inventory.

## What is missing

1. **A hook in the port that adds a part** (`mods/<mod>/parts/...`): append the `PartDef` row, add the
   two part-list entries, give the names a text id, serve the two icon textures, and put the part into
   the inventory of a new game and of a loaded save.
2. **The zone builder**: turn `blackhole.json` and the sheets into a part zone. The plan is to patch at
   load time: the new entries name the zone of a plain, unskinned head of the player's own disc, and when
   the game reads that zone for the black hole the port writes this mod's vertices, display list and
   CMPR textures over the template's and sets the definition id to the new key's hash. Nothing of the
   template is shipped.
3. Additive, unlit drawing of the disc, the bent light, the photon ring and the field (the groups named
   in `"additive"`), and the disc's spin.
4. All checks in the game: parts list, on a monster, in a battle.
