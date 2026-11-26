# Mods

The PC port loads mods from a `mods` folder next to `monsterlab.exe`. A mod is one folder in it. Nothing
on your disc image is changed: a mod's files are read in place of the disc's files while the game runs.

```
monsterlab.exe
monsterlab.ini
mods\
  MyMod\
    files\                  replaces files of the disc, by their disc path
      catalog.sir
      streams\...
    data\                   JSON that changes game data (the text table)
      strings.json
    textures\               PNG or DDS pictures that replace game textures
      sky_4962D17B32AB899E.png
```

## Replacing a game file

Put the file under `mods\<mod name>\files\` at the path it has on the disc. The game then reads that file
when it opens the disc path, whether it runs from an `.iso`, `.wbfs` or `.rvz` image or from an extracted
folder. Upper and lower case in the path do not matter.

The disc's paths are the ones under `files\` of an extracted disc (for example with Dolphin: right-click
the game, Properties, Filesystem, Extract Entire Disc). `main.dol` and the other system files are not
read through this and cannot be replaced.

A replacement must be in the format the game expects for that file (a `.sir` file is still big-endian
console data; the port converts it after reading, as it does for the disc's own).

## Replacing a texture with a PNG or DDS

A PNG (or a [DDS](#dds-for-big-packs)) under `mods\<mod name>\textures\` (sub-folders are fine) replaces one game texture. Which one is
told in one of two ways:

- **By name** (stable, the better choice): the file is called after the texture's name in the game's data,
  `MonsterLab_Logo.png` (case does not matter). `--dump-textures` writes the name into the file name when it
  knows it: `tex_256x128_f6_MonsterLab_Logo_FF636077E97CEA31.png`; rename it to `MonsterLab_Logo.png`.
- **By hash**: the **last 16 characters of the file name** are the texture's hash: `sky_4962D17B32AB899E.png`
  and `tex_256x256_f14_4962D17B32AB899E.png` replace the same texture; what stands before the hash is yours.
  The hash is the fallback: a texture without a name, or whose name no mod has, is looked up by hash.

When both match, the name wins. Names are known for the textures of `global_binary.sir` and of the level
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
- A folder that is not in the list yet is added at the end, switched on, the next time the game starts.
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
| `--verbose` | lists, at the start, every file each mod replaces and which mod wins where two have the same file; logs `mods: <file> comes from <mod>` each time the game opens a replaced file |

## Worked example: replace one file

1. Extract your own disc (or just the one file) with Dolphin.
2. Make `mods\MyMod\files\` next to `monsterlab.exe`.
3. Copy the file you changed to the same path under it, e.g. `mods\MyMod\files\catalog.sir`.
4. Start the game with `--verbose`. The log shows

   ```
   [port] mods: 1 on, from ...\mods
   [port] mods: MyMod replaces catalog.sir
   [port] mods: catalog.sir comes from MyMod
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

Do not share files of the game itself: a mod should hold only what you made or changed.
