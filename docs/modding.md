# Mods

The PC port loads mods from a `mods` folder next to `monsterlab.exe`. A mod is one folder in it. Nothing
on your disc image is changed: a mod's files are read in place of the disc's files while the game runs.

```
monsterlab.exe
monsterlab.ini
mods\
  MyMod\
    mod.ini                 name, description, priority (optional)
    files\                  replaces files of the disc, by their disc path
      catalog.sir
      streams\...
    data\                   JSON that changes game data (the text table)
      strings.json
    patches\*.json          changes to the game's tables, new parts
    textures\               PNG or DDS pictures that replace game textures
      sky_4962D17B32AB899E.png
```

## Start with the sample mod

```
python tools/make_sample_mod.py
```

writes `SampleMod` into `build\pc\mods` (`--out DIR` for another mods folder): a `mod.ini`, a
`data\strings.json` that renames *New Game* to *Modded Game*, a striped picture in place of the logo, and a
`patches\parts.json` that adds a part called *Sample Head* (see [table patches](#changing-and-adding-game-data-table-patches)).
Everything in it is made by the script. With `--data <your extracted disc>` it also copies
`files\text\str_en.txt` from your disc into the mod (one empty line added), to show a disc file being
replaced: that copy is the game's own text, so do not share it.

## Describing a mod: mod.ini

`mods\<mod name>\mod.ini` is optional, and so is every line in it:

```ini
[mod]
name = Sharper castle
description = New stone textures for the castle rooms.
author = Your name
version = 1.2
priority = 10
enabled = 1
```

| Key | Meaning |
|---|---|
| `name` | shown on the Mods page instead of the folder's name |
| `description`, `author`, `version` | shown with it |
| `priority` | where the mod goes in the load order when it is **new**: a higher number is later in the list, and wins. Default 0 |
| `enabled` | `0`: a new mod starts switched off. Default 1 |

`priority` and `enabled` only count the first time the game sees the folder. After that the player's own list
(below) decides, so an update of a mod never switches it back on or moves it. The folder's name is what
identifies a mod: renaming the folder makes it a new mod.

## Replacing a game file

Put the file under `mods\<mod name>\files\` at the path it has on the disc. The game then reads that file
when it opens the disc path, whether it runs from an `.iso`, `.wbfs` or `.rvz` image or from an extracted
folder. Upper and lower case in the path do not matter.

The disc's paths are the ones under `files\` of an extracted disc (for example with Dolphin: right-click
the game, Properties, Filesystem, Extract Entire Disc). `main.dol` and the other system files are not
read through this and cannot be replaced.

This covers every file the game reads from the disc: level and data files (`.sir`), music and voices
(`.dsp`, `.pcm` and the voice banks), movies (`fmv\*.bik`) and the text files. A mod may also hold a file the
disc does not have. The log has one line for every file replaced, the first time the game opens it:

```
[port] mod: MyMod overrides Streams/global_binary.sir
```

A replacement must be in the format the game expects for that file (a `.sir` file is still big-endian
console data; the port converts it after reading, as it does for the disc's own).

## Replacing a texture with a PNG or DDS

A PNG (or a [DDS](#dds-for-big-packs)) under `mods\<mod name>\textures\` (sub-folders are fine) replaces one game texture. Which one is
told in one of three ways:

- **By name** (stable, the better choice): the file is called after the texture's name in the game's data,
  `MonsterLab_Logo.png` (case does not matter). `--dump-textures` writes the name into the file name when it
  knows it: `tex_256x128_f6_MonsterLab_Logo_FF636077E97CEA31.png`; rename it to `MonsterLab_Logo.png`.
- **By hash**: the **last 16 characters of the file name** are the texture's hash: `sky_4962D17B32AB899E.png`
  and `tex_256x256_f14_4962D17B32AB899E.png` replace the same texture; what stands before the hash is yours.
  The hash is the fallback: a texture without a name, or whose name no mod has, is looked up by hash.

- **By a Dolphin pack's name**: `tex1_<width>x<height>[_m]_<hash>[_<palette hash>]_<format>.png` (or `.dds`),
  the names of Dolphin's custom texture packs, so a pack made for this game in Dolphin can be copied into
  `mods\MyMod\textures\`. `_m` is ignored, `$` stands for any palette (or any pixels, for paletted textures).
  Separate mip files (`..._mip1`) are not read: use a DDS with mip levels. The names follow Dolphin's current
  scheme; it has been checked against the scheme's description, not yet against a pack dumped from Dolphin.
  Dolphin's older `GAMEID_...` names are not understood.

When more than one matches, the name wins, then the 16-character hash, then the Dolphin name. Names are known for the textures of `global_binary.sir` and of the level
files; textures of other files have a hash only, for now.

To get the originals with their hashes, export them from your own disc:

```
monsterlab.exe --dump-textures C:\MonsterLabTextures
```

Or switch it on while you play: the settings screen (F1 or Esc), page **Mods**, tick **Dump textures**.
From then on every texture the game draws with is saved, including the ones on screen at that moment, and
the page counts them. They go to `dump\textures` next to `monsterlab.exe`, or to the folder typed or chosen
there. Untick it to stop. Both are kept in `monsterlab.ini` (`[textures] dump` and `folder`), so dumping
stays on at the next start until you untick it. Dumping can make the game hitch when many new textures
appear at once.

Every texture the game draws with while it runs is written there once (a file that is there already is not
written again), as
`tex_<width>x<height>_f<format>_<name>_<hash>.png` (without `_<name>` where the name is not known). Only what was on screen gets dumped: play through the
places you want to change. The dumped pictures are the game's own art: use them to work from, do not
share them, and never put them into this repository.

Rules for a replacement:

- PNG, 8 or 16 bits per channel, grey, RGB or palette, with or without transparency, **not interlaced**.
- Any size: the picture is stretched over the same surface, so 4x the width and height gives a sharper
  texture. Keep the shape (width : height) of the original.
- The hash is of the texture as the game holds it (size, format, pixels, palette), so it is the same for
  everyone with this game. A texture the game changes while it runs (a few animated ones) has a new hash
  for every state and cannot be replaced this way.
- The game still lights, tints and fogs the texture as before: a replacement on a dark, tinted surface
  comes out dark and tinted too.

### DDS, for big packs

A `.dds` file works wherever a `.png` does: `mods\MyMod\textures\MonsterLab_Logo.dds` or
`sky_4962D17B32AB899E.dds`, matched by the same name or hash rules. When one mod has both a `.png` and a
`.dds` for a texture, the `.dds` is used.

A PNG has to be unpacked and takes 4 bytes per pixel of video memory. A DDS holds the picture already
compressed the way the graphics card reads it: it loads much faster, takes 0.5 byte (BC1) or 1 byte (BC7,
BC3) per pixel of video memory, and carries smaller versions of itself (mip levels) that keep it from
shimmering at a distance. (On disk a DDS is not necessarily smaller than the PNG; a zipped pack shrinks
well.) For a pack with many large pictures, ship DDS.

Convert a pack with the tool in this repository (Python with `numpy` and `Pillow`):

```
python tools/png2dds.py C:\path\to\mods\MyMod
```

It writes `name.dds` next to every `name.png`, with all mip levels: **BC7** where the picture has
transparency, **BC1** where it is opaque. Keep the PNGs as your originals (the game ignores a PNG that has
a DDS beside it); run the tool again after editing and only the changed pictures are converted.

| Option | Effect |
|---|---|
| `--format bc7` | BC7 for everything: better than BC1 on gradients and dark skies, twice the size (also `bc3`, `bc1`) |
| `--out DIR` | write the DDS files under `DIR` (same sub-folders), for a release folder without PNGs |
| `--delete-png` | delete each PNG once its DDS is written |
| `--force` | convert again even if the DDS is newer |
| `--no-mips` | the full-size picture only |

DDS files from other tools (for example `texconv`) work too. Accepted: a 2D texture in BC1 (DXT1), BC3
(DXT5) or BC7, width and height multiples of 4, up to 8192. The mip levels in the file are used as they
are (none are made for a DDS, whatever the filter setting). sRGB variants are read as plain ones. BC7
needs a Direct3D 11 graphics card.

### 16:9 art for backgrounds and loading screens

The game draws its 2D layer for a 4:3 screen. When the port runs at 16:9 it keeps 2D pictures in their
shape, in the middle of the screen. A replacement can be painted wider instead, and then fills the screen.
The rule: the quad is widened to the full width when **all** of these hold:

1. the game runs at 16:9 (and *Stretch the 2D layer* is off; with it on everything is stretched anyway);
2. the picture is drawn flat (2D) over the **whole screen**, edge to edge in both directions;
3. the game's own texture is **320 pixels wide or more** (smaller full-screen textures are stretched to the
   width already);
4. the replacement's width : height is **at least 1.2 times** the original's. For example 1.333 times for 4:3
   art redrawn at 16:9: a 640x480 original (4:3) replaced by 2560x1440 (16:9), or a 512x512 original by a
   picture with a 4:3 shape.

So paint the original's content in the **middle three quarters** of the new width and add scenery at the
sides: buttons and figures drawn on top of the background stay where they were. A picture that the game
builds from several tiles, or draws over part of the screen only, is not widened. At 4:3 the same file is
squeezed back into the 4:3 picture.

### Big packs

The files are read and unpacked in the background, so the game does not stop while a pack loads: a texture
shows the game's own picture for the moment it takes to read its replacement. What has been read is kept in
memory (up to 256 MB, the longest unused goes first), and from the start of the game the pack is read
ahead until that is nearly full. At most about 24 MB of new pictures are handed to the graphics
card per frame. DDS files are by far the quickest to read, since nothing has to be unpacked.

A file edited while the game runs is read again after **Look again** on the Mods page.

With `--verbose` the log says `mods: texture <hash> comes from <mod>` the first time a replacement is
used. A replacement found by name is logged as `mods: texture <name> (<hash>) comes from <mod> by name`. A PNG or DDS that cannot be read is reported and the game's own texture is drawn.

## Changing the game's text with JSON

`mods\<mod name>\data\strings.json` changes entries of the game's text table (4,274 strings: menu and
HUD text, part and monster names, descriptions). The game reads a string by its number, so a file lists the
numbers it changes:

```json
{
  "strings": {
    "1552": "Start Game",
    "1404": "Resume"
  }
}
```

Get the original text from your own disc (this writes one file and exits; it is the game's own text, do not
share it and never commit it):

```
monsterlab.exe --dump-data C:\MonsterLabData
```

`C:\MonsterLabData\strings.json` has the form above with every string (`"format": 1`, `"table": "strings"`,
`"count"` and `"strings"`). Keep only the lines you change in your mod's file, or copy the whole file; both work.

Rules, checked when the game loads `global_binary.sir`:

- Keys are string numbers in quotes, `0` to `count - 1`; values are text. Standard JSON escapes work
  (`\n`, `\"`, `\u00E9`). The text is single-byte: characters up to U+00FF only, as the game's font has them.
  Special codes of the game stay as they are in the dump (for example `$newline`).
- Unknown keys, a string number that does not exist, a character above U+00FF or a syntax error make the
  **whole file ignored**, with a message that names the file, the line and the column:
  `mods: MyMod: ...\strings.json: line 3, column 5: string 99999 does not exist (the game has 4274: 0-4273); the file is ignored`.
- With several mods the later one in the load order wins per string.
- The text is read when the game starts: restart after a change.

## Adding a part with its own model

A new part is a table patch (see [Adding a part](#adding-a-part) below): that gives it a row, names and saves.
Its model needs more. For a head there is one way so far: `mods/<mod name>/parts/<model key>.part` holds the
mod's own vertices and pictures and names a part of your disc as its template. While the new part is on a
monster the port draws them in place of the template's, in memory; skeleton, animations and material are the
template's. The mod's patch must add that part with the id `head`. Nobody is given the part yet.

`tools/mods/blackhole` is the worked example (a black hole head): `make_assets.py` draws the pictures and
the model, `build_mod.py` writes the `.part` file (its header describes the format) and the patch, and the
README says how a part is stored and what does not work yet (no icon of its own, no model on the exploration
map).

## Load order, switching mods on and off

`monsterlab.ini` keeps the list:

```ini
[mods]
BaseMod = 1
MyMod = 1
OldMod = 0
```

- One line per mod: `1` is on, `0` is off.
- The order of the lines is the load order. When two mods replace the same file, the one **later** in the
  list wins.
- A folder that is not in the list yet is added the next time the game starts: in front of the first mod
  with a higher `priority` in its `mod.ini` (without priorities: at the end), switched on unless its
  `mod.ini` says `enabled = 0`.
- A mod in the list whose folder is gone is skipped.
- Textures follow the same order: when two mods replace the same texture, the later one wins.

## The Mods page

The settings screen (F1 or Esc in the game's window) has a **Mods** page: a tick box switches a mod on or
off and the arrows move it up or down in the load order. A change is written to `monsterlab.ini` at once
and counts for every file the game opens from then on; a file that is loaded already stays as it is until
the game loads it again (a level change, or a restart for the files read at the start). Textures change
at once.

## Command line

| Option | Effect |
|---|---|
| `--mods DIR` | use `DIR` as the mods folder instead of `mods` next to the exe |
| `--no-mods` | run without mods |
| `--dump-data DIR` | write the game's text table to `DIR\strings.json` and exit (from your own disc) |
| `--dump-textures DIR` | write every texture the game draws with to `DIR` as a PNG named by its name (where known) and hash (the same as *Dump textures* in the settings, for this run) |
| `--verbose` | lists, at the start, every file each mod replaces and which mod wins where two have the same file |

## Worked example: replace one file

1. Extract your own disc (or just the one file) with Dolphin.
2. Make `mods\MyMod\files\` next to `monsterlab.exe`.
3. Copy the file you changed to the same path under it, e.g. `mods\MyMod\files\catalog.sir`.
4. Start the game with `--verbose`. The log shows

   ```
   [port] mods: 1 on, from ...\mods
   [port] mods: MyMod replaces catalog.sir
   [port] mod: MyMod overrides catalog.sir
   ```

## Worked example: a red sky

1. Export the textures of the main menu from your own disc. This runs without a window to the menu and stops:

   ```
   monsterlab.exe --to-menu --frames 3300 --dump-textures C:\MonsterLabTextures
   ```

2. Find the night sky in `C:\MonsterLabTextures`: `tex_256x256_f14_4962D17B32AB899E.png`.
3. Open it in a paint program, tint it red, and save it as
   `mods\RedSky\textures\sky_4962D17B32AB899E.png` next to `monsterlab.exe` (keep the 16 characters
   before `.png`).
4. Start the game. The sky behind the main menu is red; `--verbose` logs

   ```
   [port] mods: RedSky has 1 texture
   [port] mods: texture 4962D17B32AB899E comes from RedSky (256x256, ...\sky_4962D17B32AB899E.png)
   ```

5. Open the settings screen (F1), page Mods, and untick RedSky: the sky is the game's own again.

## Worked example: rename the menu

1. `monsterlab.exe --dump-data C:\MonsterLabData`, then look up the menu words in `strings.json`: `"1552": "New Game"`.
2. Make `mods\Words\data\strings.json` with just `{"strings": {"1552": "New Monster"}}`.
3. Start the game with `--verbose`: `mods: Words changes 1 string (data/strings.json)`. The main menu says
   "New Monster".

`python tools/mod_test.py` does these steps by itself in a temporary folder (dump, tint one texture red, run,
compare the screenshots; the same by name; dump the text, change menu text, a broken file); it is the test for this feature.

### The same by name

If the dump gave the texture a name (`tex_256x128_f6_MonsterLab_Logo_FF636077E97CEA31.png`), save your
version as `mods\MyMod\textures\MonsterLab_Logo.png` instead. `--verbose` logs
`mods: texture MonsterLab_Logo (FF636077E97CEA31) comes from MyMod by name`. `tools/mod_test.py` checks this too.

## Changing and adding game data: table patches

`mods\<mod name>\patches\*.json` changes the game database (the 42 tables of `global_binary.sir`: parts,
actions, tuning and more) in memory, after the game has loaded it from your disc. A mod holds only its own
entries, never the file, and two mods can add to the same table.

```json
{
  "tables": [
    {"table": 22, "row": 73, "set": {"@0x4A": 2}},
    {"table": 5, "append": true, "copy": 3, "set": {"#1": 40}}
  ],
  "parts": [
    {"id": "head", "copy": 73,
     "set": {"@0x4A": 77},
     "names": ["Black Hole", "Dark Hole", "Deep Hole", "Singularity"],
     "data": {"model": "parts/hole.part"}}
  ]
}
```

- `table` and `row` are numbers from 0. A column in `set` is `"#<column number>"`, `"@<byte offset in the
  row>"` or the column's own name (the tables store a hash of each name). Values are numbers; for a hash
  column a string is hashed as the game does it.
- `append` adds a row that starts as a copy of row `copy`. Appended rows get their numbers from the load
  order, so use them only for things a save does not remember. Parts are refused here: use `parts`.
- Mods apply in load order, every mod's `tables` before any `parts`, so a new part copies the patched row.
  Later changes win.
- An entry with a mistake (unknown column, a value that does not fit) is ignored as a whole; a file that is
  not valid is ignored as a whole. The log says why, with line and column for the file.
- Changes need a restart.

### Adding a part

Each entry of `parts` is a new row of the part table (table 22, `PartDef` in `include/game/part.h`):

| Key | Meaning |
|---|---|
| `id` | letters, digits, `_` and `-`. The part's permanent key is `<mod folder>:<id>`: do not rename either |
| `copy` | the vanilla part (0-185) it starts from, and what it looks like to a player without the mod |
| `set` | columns to change, as above |
| `names` | up to four names, one per tier; tiers without one keep the copied part's |
| `data` | anything the port's own code for this part wants; the table patcher only passes it on |

A new part is only a definition: this does not give it to the player, or give it a model or an icon. Code
that does asks `include/port/port_modpatch.h` (`PortModPartIndex("Mod:head")`, `PortModPartAt`,
`PortModPartData`).

### Saves and `save.modparts`

A save remembers a part as its row number, in one byte. So a modded part keeps its row for good, and the
save format is unchanged: `save.modparts` in the save folder (next to the save) lists
`row|vanilla row|mod:id`. A new part takes the next free row from 186 on; the line is only ever added, never
changed or removed. Reordering mods, adding mods and removing mods therefore never moves a part.

While a part's mod is off or gone, its row holds the vanilla part it was copied from: the player keeps the
part, it behaves as that vanilla part, it is saved under its own number again, and it is the mod's part
once the mod is back. Keep `save.modparts` with the saves when you move them. Without it a save with modded
parts still loads (unknown rows show as vanilla part 0), but the parts can then be matched to the wrong mod.
There is room for 69 modded parts per save folder (rows 186-254).

Not covered by table patches: level part lists and zones, models and icons (they are not in the game
database), and new text other than a part's names (`strings.json` changes existing strings).

## Testing

`monsterlab.exe --selftest` checks the lookup, the load order, `mod.ini`, a mod's file opened through the
game's own disc functions, and table patches, added parts and `save.modparts` across adding, removing and
reordering mods (no game data needed). `python tools/smoke.py` runs the menu with the sample mod
and fails unless the game took its text, its new part and, with an extracted disc, its file.

Do not share files of the game itself: a mod should hold only what you made or changed.
