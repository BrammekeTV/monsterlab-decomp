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
    textures\               PNG pictures that replace game textures
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

## Replacing a texture with a PNG

A PNG under `mods\<mod name>	extures\` (sub-folders are fine) replaces one game texture. Which one is
told by the **last 16 characters of the file name**, the texture's hash: `sky_4962D17B32AB899E.png` and
`tex_256x256_f14_4962D17B32AB899E.png` replace the same texture; what stands before the hash is yours.

To get the originals with their hashes, export them from your own disc:

```
monsterlab.exe --dump-textures C:\MonsterLabTextures
```

Every texture the game draws with while it runs is written there once, as
`tex_<width>x<height>_f<format>_<hash>.png`. Only what was on screen gets dumped: play through the
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
- Matching is by hash only for now, not by the texture's name inside the game's files.

With `--verbose` the log says `mods: texture <hash> comes from <mod>` the first time a replacement is
used. A PNG that cannot be read is reported and the game's own texture is drawn.

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
| `--dump-textures DIR` | write every texture the game draws with to `DIR` as a PNG named by its hash |
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
   `mods\RedSky	extures\sky_4962D17B32AB899E.png` next to `monsterlab.exe` (keep the 16 characters
   before `.png`).
4. Start the game. The sky behind the main menu is red; `--verbose` logs

   ```
   [port] mods: RedSky has 1 texture
   [port] mods: texture 4962D17B32AB899E comes from RedSky (256x256, ...\sky_4962D17B32AB899E.png)
   ```

5. Open the settings screen (F1), page Mods, and untick RedSky: the sky is the game's own again.

`python tools/mod_test.py` does the same steps by itself in a temporary folder (dump, tint one texture
red, run, compare the screenshots); it is the test for this feature.

Do not share files of the game itself: a mod should hold only what you made or changed.
