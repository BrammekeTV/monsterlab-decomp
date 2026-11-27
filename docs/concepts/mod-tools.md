# Mod tools: `.sir` editor and `.part` viewer (design)

Status: design, step 1 under way. The modder's guide is `docs/modding.md`.

## Goal

Modders work on Monster Lab content without hex editors or Python dumps: open any `.sir` of their
own disc, see what is in it (typed, with names where the game gives them), look at textures,
meshes and strings, preview a `.part` mod with live reload, change values and save the result as
a mod. The tool never writes to the disc image or `orig/`, and nothing it shows ever goes into the
repository.

## Shape: one exe, two names

`ml_modtool.exe` is **the port's own executable started as the tool**. `tools/build_pc.py` (and
the CMake build) puts a copy of `monsterlab.exe` named `ml_modtool.exe` next to it; the exe looks
at its own file name (or `--modtool`) and runs the tool instead of the game.

Why not a second, separately linked executable (the shape first suggested):

- **Parsing and drawing as the game does means the game's code.** The `.sir` loader
  (`sir_loader.cpp`), the disc reader (`port_disc.cpp`, `.iso/.wbfs/.rvz`), the mods layer
  (`port_mods.cpp`, `port_modpatch.cpp`, `port_mod_parts.cpp`), the GX layer (`port_gx*.cpp`) and the
  D3D11 renderer are all in the one image that `tools/port_link.py` links at a fixed base with
  absolute addresses (`port_gen.obj`). The 3D view draws meshes by feeding their display lists and
  material GX state through that same GX layer, and the `.part` view uses the same filter
  (`PortGxFilter`) as the game. A second link would repeat 850 objects and the link glue, and
  could drift from the game's build.
- **One build, one test path.** CI already builds and self-tests `monsterlab.exe`; the tool's
  self-test runs in the same `--selftest` (no game data) plus `--modtool-test` (with data).
- The copy costs a few MB on disk and nothing in the build.

The tool starts before the game's boot (`PortBoot`, right after the disc or folder is opened and
`main.dol` is in place), runs its own window with Dear ImGui (Win32 + DX11 backends, as the
first-run picker does), and exits. The game's `main()` never runs in tool mode, so no game thread,
no interrupts, no sound.

Headless use (tests, screenshots for the docs) follows the picker: `--modtool-shot FILE.png` draws
the screen into a PNG without a window, `--modtool-open PATH` and `--modtool-select NODE` choose
what it shows.

## Data sources

| Source | How it is read |
|---|---|
| Disc image (`--iso`, `[game] path`) | `PortDiscFind` / `PortDiscRead` (the game partition's file table) |
| Extracted folder (`orig/RM7P4F`, `--data`) | `<data>/files/<disc path>` |
| Mod folder (`mods/<mod>/files/...`) | plain files; shown as an overlay of the disc path they replace |
| Any file (`--modtool-open FILE`, drag and drop) | plain file |

The file list is the disc's `files/` tree (from the catalog's 59 `.sir` names under `streams/`
plus the text files), with the enabled mods' replacements marked. Packed copies (`...C.sir`) are
listed but opened through their unpacked twin.

## Document model

An opened file keeps **two copies**: the raw big-endian bytes as on disc, and a copy converted by
the port's loader (`GlobalBinaryConvert`, `LevelHeaderConvert`/`LevelZoneConvert`,
`SirCatalogConvert`, ...). Every node of the tree knows its byte range in the raw copy, so the hex
view shows the disc bytes and the typed view shows what the game reads from them (host pointers
mapped back to file offsets). A file the loader refuses still opens as a container (sections and
hex) with the loader's error shown.

Tree, by file kind:

- every `.sir`: container header, section table (type, offset, size, hash);
- `global_binary.sir`: header slots (the known ones named after `sir_loader.h`), game database
  tables (rows x typed columns), texture list, string table, action table, UI root and fonts,
  screen state machines, effect tables;
- `*_level.sir` and rooms: world-state header, zone slots, zones (entities, scene items, meshes,
  materials, textures), as far as the loader types them;
- `catalog.sir`: entries and file records.

Names come only from the game: a hash is shown with the string behind it when one of the strings
of `main.dol` or the open file hashes to it (`StringHash`, CRC-32 of the upper-cased text),
else as `#index` / hex, the same spellings the patch format accepts.

## Viewers (step 2)

- **Textures**: decoded by the renderer's own decoder (`DecodeTexture`, all GX formats, palettes),
  zoom and mip level, export to PNG (`port_png.cpp`) or DDS (`port_dds.cpp`), with the file name
  a texture pack expects (`<name>_<hash>.png`).
- **3D**: a mesh, scene item or zone drawn into an off-screen target through the GX layer with its
  material's GX state, orbit camera (drag to turn, wheel to zoom), wireframe and texture toggles.
- **Strings**: the string table with ids, search, and the JSON key a `data/strings.json` uses.

## `.part` viewer (step 3)

Loads `mods/<mod>/parts/*.part` (version 2, the format of `tools/mods/blackhole/build_mod.py` and
`port_mod_parts.cpp`): mesh, colour sheet and the blocks of light draws. Shown alone or attached to
a template monster's slot, with the light draws' glow and spin animated by the tool's clock. The
file is watched (`FindFirstChangeNotification` on its folder) and reloaded when it changes, so
`build_mod.py` and the view form a loop of seconds.

## Editing and saving as a mod (step 4)

- Typed fields of game database rows, strings, part stats: edited in place in the converted copy;
  every edit is recorded as `{table, row, column, old, new}`.
- Saving writes **only** into `mods/<mod>/`:
  - table edits: `patches/modtool.json` in the format of #286 (`{"tables": [{"table", "row",
    "set"}]}`), columns by name when the game has one, else `#index`;
  - string edits: `data/strings.json`;
  - texture replacements from a PNG: `textures/<name>_<hash>.png`;
  - anything else changed in a file: the whole file under `files/<disc path>`.
- A **diff** page lists what the mod changes against the disc: patch rows (old -> new), strings,
  textures, replaced files.
- The tool refuses any save path outside the mods folder and never opens disc sources for
  writing.

## Tests

- `--selftest` (CI, no game data): the tool's document model on a small `.sir` made in memory
  (container, sections, a game-database table), and a patch round trip: edit -> JSON -> the
  patch code of the game (`PortModPatchRun`) -> the same values.
- `--modtool-test` (local, with the player's disc; `tools/modtool_test.py`): opens
  `catalog.sir`, `global_binary.sir` and a level file, checks counts against the loader, makes a
  mod with one table edit in a temporary folder, loads it the way the game does and compares.

## Steps (one PR each)

1. Design, tool mode, open and browse (tree, hex + typed fields), headless screenshot, tests.
2. Texture, string and 3D viewers; PNG/DDS export.
3. `.part` viewer with live reload.
4. Editing, save as mod, diff.
5. Modder's guide in `docs/modding.md` with a worked example.
