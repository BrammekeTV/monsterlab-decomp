# Black hole head (mod)

A new head part for the PC port: a black ball inside a larger amber ball that is turned inside out (an
"inverted hull"), so only the far inner side of the amber ball is drawn and the black ball has a glowing,
streaky rim from every side. Under it, on the neck, a small stabilizer with an amber cap. No bolts, no lab
parts on the hole itself.

```
python tools/mods/blackhole/make_assets.py     # pictures and model   -> build/mods/BlackHoleHead
python tools/mods/blackhole/build_mod.py       # the mod folder       -> build/pc/mods/BlackHoleHead
```

Needs `numpy` and `Pillow`. Nothing of the game is read and nothing of the game is in the mod: pictures,
model and draw commands are made by these scripts. Copy `BlackHoleHead` into the `mods` folder next to
`monsterlab.exe` (a build's own folder is `build/pc/mods`). The part is then in the inventory of every game,
new or loaded, as "Black Hole" (tier 1, the higher tiers are named too).

| File | What |
|---|---|
| `make_assets.py` | draws everything from code: the 256x256 colour and gloss sheets, the 64x64 and 256x256 part icons, the model (392 vertices, 652 triangles: core, hull, emitter) as `.obj` and `.json` |
| `build_mod.py` | packs model and sheets into `parts/p_hd_sha077.part` (the format is in its header) |
| `sirlib.py` | small reader for the game's `.sir` containers, for looking at a template |

## How it gets into the game

The port's side is `src/port/port_mod_parts.cpp` (see also `docs/modding.md`, "Adding a part"):

- **Definition.** The part table (table 22 of the game database, `PartDef`, 0x50 bytes, 186 rows) gets row
  186: a copy of the row of a plain head of the player's disc, the "template" `p_hd_sha002` (row 73), with
  model number 77. `PartAnimKey` builds the model key from the animation set and that number, so the key is
  `p_hd_sha077` and every head animation is reused.
- **Model.** The level's part list (level header `+0x44`, entries `{users, name hash, object, zone}`, two per
  part: one per fighter) gets two entries for `p_hd_sha077_geoA` that name the template's zones (23 and 24 of
  `shared_parts.sir`). While the black hole is the part in use in such a zone, the port writes the mod's
  vertex arrays, draw commands and the two CMPR sheets over the template's in memory and sets the zone's name
  hash to the new key's (the game finds the list entry to release by that hash). When the template itself is
  in use again, the bytes are put back.
- **Names.** Four new text ids behind the game's string table; `TextIdFromHash` asks the port for keys it
  does not know.
- **Owning it.** A game whose inventory has no such part gets one.

What the model keeps of the template: its skeleton and animations, its material (lit, environment map, rim
light), the matrix commands of its display list. The hull is drawn by that same material with the usual
back-face culling (the material's GX state record ends with depth and cull settings; the word at `+0xD18`
is 2, back, and there is one record per material, not per group): its triangles are wound the other way and its normals point inward, which gives the inside-out
look without a second material.

## Found on the way

- A part's zone is read from disc only when another zone took its place in memory (one address per limb and
  fighter): putting on the same part again does not read it again. So the port checks every frame which of
  the two (template or mod) a zone must show, not only at the read.
- 186 is fixed in two places: `PART_DEFS` in `src/game/battle_core.cpp` (stepping to the next part of
  the same limb: it never reaches row 186) and `fn_8020E438` (a name per row, with a fallback for
  other rows). A save stores a part's row in one byte (0xFF = none), so row 186 fits. A save made with the
  mod has a part of row 186: load it without the mod and that row does not exist.

## What is left

- **Icon.** The two icon pictures are made, but the game still shows its placeholder: the icon is a section
  of `shared_texture.sir` found through the catalog, and the port cannot add a section yet.
- **Map model.** No low-detail entry (`_lr`): on the exploration map the game falls back to head 001 and logs
  "The part list is missing a part it should have".
- **Unlit, additive, spin.** The hull is lit like the rest (one material). A second material, or a material
  of the mod's own, would make it full bright and let its picture scroll.
- **Description text** and a place in the shop or as a reward: it is only given to the inventory.
