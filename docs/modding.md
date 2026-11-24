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

## Command line

| Option | Effect |
|---|---|
| `--mods DIR` | use `DIR` as the mods folder instead of `mods` next to the exe |
| `--no-mods` | run without mods |
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

Do not share files of the game itself: a mod should hold only what you made or changed.
