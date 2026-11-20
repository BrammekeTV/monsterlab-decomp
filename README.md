# Monster Lab (Wii) — decompilation / PC port

Target: **RM7P4F** (PAL), `main.dol` SHA-1 `50f6e9b94dbe4df6f663541493106d19e0a40c72`.

Bring your own disc: the extracted game lives in `orig/RM7P4F/` and is git-ignored.

## Layout

| Path | Contents |
|---|---|
| `orig/RM7P4F/sys/main.dol` | Game executable (the only code binary; there are no RELs) |
| `orig/RM7P4F/files/` | Game data (2.5 GB) |
| `config/RM7P4F/` | decomp-toolkit config, symbols, splits, target hash |
| `src/`, `include/` | Decompiled C/C++ |
| `tools/` | Helper scripts |

## What's in the binary

- Compiler: Metrowerks CodeWarrior (PowerPC), MSL runtime + "Metrowerks Target Resident Kernel".
- SDK: RVL_SDK **RevoEX 2.1plus**, build `080313085908`: OS, GX, AX/AI/DSP, DVD, NAND, WPAD/KPAD, HBM (Home menu), SC, VI, plus the networking stack (DWC, NHTTP, SSL, SOCKET, NWC24).
- Third-party: **Bink** video (RAD Game Tools).
- Engine: an in-house cross-platform engine with `Is_WiiBuild` / `Is_DSBuild` / `IsFinalBuild` flags and a `SOUNDENGINE_*` audio layer. Data is stored in `.sir` containers (magic `PNP0`).
- No symbol map ships on the disc, so all symbols have to be recovered (SDK signatures first).

DOL sections:

| Section | Address | Size |
|---|---|---|
| .init (T0) | 0x80004000 | 0x24E0 |
| .text (T1) | 0x80006B40 | 0x33EFA0 |
| D0–D7 | 0x800064E0 – 0x80600FE0 | — |
| .bss | 0x803B5380 | 0x24BD98 |
| Entry point | 0x8000403C | |

## Data formats

| Ext | Count | Format | PC replacement |
|---|---|---|---|
| `.bik` | 78 | Bink video | Bink SDK for PC, or FFmpeg (`libavcodec` decodes Bink 1) |
| `.dsp` | 89 | Nintendo DSP-ADPCM audio | Decode to PCM (vgmstream or a small decoder) |
| `.sir` | 60 | Engine container, magic `PNP0`: sections that are raw memory images (see below) | Load as-is; pointers fixed up on load |
| `.arc` / `.tpl` | 10 | Nintendo U8 archives / textures (Home Button menu) | Not needed on PC |
| `.mid`, `.wt`, `.pcm` | — | MIDI music + wavetable | Software synth |

### `.sir` memory layout

Each `.sir` section is a memory image the game copies to a fixed address in MEM2; pointers inside are absolute Wii addresses. Load addresses found with `tools/sir_decoder.py --list` over every archive:

| Address | Contents |
|---|---|
| `0x90400000` | `global_binary.sir` section 1 (about 1 MB, always loaded). Section 0 is a 4 KB pointer table read into `sSirHeader`. |
| `0x90540000` | Current level/arena, section 0 (`*_level`, `*_arena`, `castle_*`, `trouble_scope_map`) |
| `0x90980000` | Current level/arena, section 1 (textures and geometry) |
| `0x91240000` – `0x922C0000` | `shared_parts.sir`: 432 monster-part sections streamed into fixed slots |
| `0x91B80000`, `0x924C0000`, `0x92940000` | `shared_ui.sir` screens |

`castle_*C.sir` (no placed sections), `shared_texture.sir` (607 sections) and `shared_voice.sir` (5,168 sections) use other layouts and still need looking at.

On PC the files stay in their original format, which keeps mods simple. The loader byte-swaps each field by its type and relocates the pointers in each section as it loads ([`docs/loader.md`](docs/loader.md)).

### Monster part data

Part types live in `global_binary.sir`. Header slot `0xDC8` points to the game database (`gGameDb`), and its data block holds the part definition table at `+0x5B78` (186 entries of 0x50 bytes; 180 are used, 180–185 are empty). Each definition (`PartDef` in `include/game/part.h`) has four tier names, e.g. *Chimp Arm → Ape Arm → Gorilla Arm → Kong Arm*. The names are stored as hashed text keys, which the table at `0x8036D370` in `main.dol` maps to text ids, and text id *N* is line *N+1* of `files/text/str_<lang>.txt` (plain text). After the names come two action ids and the stats. Parts the player owns are `PartInstance`s: definition index, tier, quality, defect, enhancement, health. `python tools/parts.py` dumps the table from your own disc into `build/`.

## Plan

1. **Split & disassemble.** `dtk dol split config/RM7P4F/config.yml build/RM7P4F` gives per-object assembly and builds up `symbols.txt` and `splits.txt`.
2. **Recover symbols.** Apply RVL SDK / MSL / Bink signatures (for example from other RevoEX-2.1 decomps) so that roughly a third of `.text` gets names.
3. **Decompile.** Use Ghidra or IDA with a Wii loader, plus m2c, to produce C. Check it against the original with objdiff (matching decomp) or skip matching (functional decomp).
4. **PC port.** Build the game code natively for PC (32-bit first, see Roadmap) and replace the Nintendo SDK layer:
   - GX → OpenGL/Vulkan backend (or a GX emulation layer)
   - AX/DSP → SDL audio + DSP-ADPCM decoder
   - WPAD/KPAD → SDL gamepad + mouse as the pointer
   - DVD/NAND → host filesystem
   - Fix endianness (all data is big-endian).

**Chosen approach: decompile by hand, then port.** The decomp is functional rather than byte-matching: game code is rewritten as portable C/C++, and the Nintendo SDK layer is replaced instead of decompiled. Static recompilation was considered and rejected.

### PC port features (planned)

These live in the platform layer (`src/platform/`) that replaces the Wii SDK, so the decompiled game code stays mostly untouched.

- **1080p and higher, widescreen.** The GX → OpenGL/Vulkan backend renders at any output resolution (1080p, 1440p, 4K) with an internal resolution scale, instead of the Wii's 640×480 framebuffer. The game already supports 16:9: `ViewCreate` picks the aspect ratio from `SCGetAspectRatio`, so on PC that returns widescreen. 2D UI, which uses fixed 640×480 coordinates, gets scaled or pillarboxed.
- **Settings GUI.** An in-game overlay (Dear ImGui, toggled with F1) plus a `config.ini`. Options: resolution, window/borderless/fullscreen, V-sync, frame-rate cap (it maps onto the game's own `gFrameLimitEnabled` / `gTargetFps`), internal render scale, MSAA, texture filtering, audio volume and controls (pointer = mouse).
- **Mod support.** A virtual file system sits between the game and the disc files: `mods/<ModName>/files/...` overrides any original file (`.sir`, `.bik`, `.dsp`, text). Load order and enabled mods come from `mods.ini` and can be toggled in the settings GUI. Later: documented `.sir` formats and tools for editing them, plus code hooks for script mods.
- **Content mods in open formats (no `.sir` needed).** An importer builds the game's in-memory data from standard files, so new content (starting with monster parts) can be added without touching `.sir`:

  ```
  mods/LaserArm/
    mod.json                 name, author, version
    parts/laser_arm/
      part.json              slot (arm/leg/head/torso), stats, actions, damage, cost
      model.glb              glTF model + skin weights (Blender export)
      texture.png, icon.png
      text/en.txt, nl.txt…   name and description per language
  ```

  Stats are served through the game's own part queries (`PartQuery_GetHealth`, `PartQuery_GetActionDamage`, ...), and text is added to the string table. New parts reuse the existing monster skeletons and sockets. The mod loader gives mod parts their own ids, and saves cope with a removed mod. An exporter turns an original part into glTF + PNG + JSON as a starting point for modders. Depends on decompiling the part definition, attachment, save and menu code.

### Roadmap (next steps)

1. **32-bit build first (done).** The decompiled code reads game structures at their original offsets with 4-byte pointer slots, which is only layout-correct in a 32-bit (Win32) build.
2. **Link step + missing-symbols report (done).** `tools/link_check.py` + `tools/missing_symbols.py`; CI links everything and lists unresolved functions/globals; that list drives the remaining decompilation.
3. **Big-endian data loader (started).** Convert game data (game database, part definitions, saves) at load time. `src/port/sir_loader.cpp` converts `catalog.sir` (the file table) and the `global_binary.sir` header and game database, using the table schemas stored in the file; design, results and the next files are in [`docs/loader.md`](docs/loader.md).
4. **Identify more SDK code.** Apply RVL SDK / MSL / Bink signatures with decomp-toolkit so the platform layer knows exactly which Nintendo SDK functions to replace (about 460 of ~4,000 library functions are named so far).
5. **Boot to the title screen (started).** The PC build links into `monsterlab.exe` and runs the game's `main()`: `python tools/build_pc.py`, design in [`docs/port_runtime.md`](docs/port_runtime.md). The exe maps the Wii address space at its original addresses, loads the data of your own `main.dol` into it and stops at the first function that is not decompiled, naming it. Today it gets through `SysInit`, the heaps, `EngineStartup` and `GameInit`, and stops in `EngineInit` at `VideoInit`. Then: compare self-contained functions against the original (differential tests).
6. **Own-disc setup (ISO picker).** The port never ships game content: models, textures, audio, level and script files, and the data in `main.dol` stay the publisher's. On first launch the player picks their own `.iso` or `.wbfs`. The port checks that it is the PAL `RM7P4F` disc, extracts and caches the files it needs, and never asks again. This replaces extracting the disc into `orig/` by hand, and fits into the settings GUI. Mods (custom parts as glTF/PNG/JSON) load on top of the player's own files.
7. **64-bit later.** Only after named structs and a loader replace raw offsets; not needed for 1080p, the settings GUI or mods.

### Priorities

- Game code (`main` is at `0x80142DF0`; game code sits at `0x80006B40`–`0x8022FEC4`) is the real work. zlib 1.2.3 follows at `0x8022FEC4`–`0x80232AF8` (the PC port links real zlib).
- SDK, MSL and Bink code (from about `0x80232AF8`) only needs to be **identified** so that it can be stubbed or reimplemented on PC. It does not need to be decompiled.

## Workflow

- `python tools/xref.py func <name|addr>` shows a function's calls, referenced strings and callers. `python tools/xref.py strings <regex>` finds functions by the strings they use.
- `python tools/decomp.py <name|0xADDR>...` writes an automatic C draft of a function to `build/m2c/` using [m2c](https://github.com/matt-kempster/m2c) (one-time setup: `git clone https://github.com/matt-kempster/m2c tools/m2c` and `pip install "graphviz~=0.20.1"`). It reads struct types and prototypes from `tools/m2c_context.h`, from every `extern "C"` declaration in `src/` and `include/`, and from `script_callbacks.h`, so known fields show up by name (re-run the dtk split after renaming so the asm uses the new names). The draft is only a starting point; types, names and logic are fixed by hand before code goes into `src/`.
- `python tools/gen_leaf.py` regenerates `src/leaf/`: small straight-line functions (getters, setters, constant returns, empty stubs, bitfield and float accessors) translated instruction by instruction. To decompile one of them properly, write it in a normal source file with its `// 0xADDRESS` marker and re-run the script; it skips functions that are already decompiled.
- `python tools/check.py [files]` compiles files locally with MSVC (Visual Studio Build Tools, C++ workload) using the CI flags; with no arguments it checks the files changed since the last commit (all of them if a header changed). The whole tree takes about 10 seconds.
- `sh tools/fn.sh <name>` prints a function's asm. `sh tools/dump.sh <name>...` prints a compact version.
- `python tools/todo.py <object>` lists an object's functions, their size and whether they're decompiled.
- `python tools/script_funcs.py` lists every script callback the game registers by name (`RegisterScriptFunc`); this is where most original function names come from.
- `python tools/hashes.py build`, then `python tools/hashes.py <0xHASH>` or `scan`: names behind `StringHash` ids (CRC-32 of the upper-cased name) found in the code.
- `python tools/sir_decoder.py <file.sir>` extracts textures, meshes and skeletons from `.sir` archives (contributed by the project owner; see `tools/sir_decoder.md`).
- `python tools/rename.py OLD NEW` (or `-f file`) renames symbols in `symbols.txt`. The `config/RM7P4F/names_*.txt` files keep a record of names and where they came from. Re-run the dtk split afterwards.
- **Decompiled-function marker:** put a line containing only `// 0xADDRESS` directly above each function definition in `src/`. `tools/progress.py` counts these.
- `python tools/progress.py --record` updates `docs/data/progress.json` and `history.json`. Milestones and the changelog live in `docs/data/features.json`.
- **Progress site:** <https://brammeketv.github.io/monsterlab-decomp/> rebuilds on every push to `master` (`.github/workflows/pages.yml`).

## Status

- `dtk dol split` runs cleanly: 12,580 functions, 707 objects, assembly in `build/RM7P4F/asm/`.
- Progress (functions, bytes, names) is on the [progress site](https://brammeketv.github.io/monsterlab-decomp/).
- Every push is compiled by GitHub CI (MSVC); `tools/check.py` runs the same check locally.

## Contributing

Want to help, by hand or with your own AI agent? See [CONTRIBUTING.md](CONTRIBUTING.md) for setup
and workflow, and point your agent at [AGENTS.md](AGENTS.md).

## Tools

- DolphinTool (already on this machine), used for disc extraction:
  `DolphinTool extract -i "RM7P4F - Monster Lab.iso" -o orig/RM7P4F -g`
- `tools/bin/dtk.exe`: decomp-toolkit v1.9.1. Re-split with
  `tools/bin/dtk.exe dol split config/RM7P4F/config.yml build/RM7P4F`
- `tools/bin/objdiff.exe` / `objdiff-cli.exe`: objdiff v3.8.3, for comparing decompiled functions against the original assembly.
- Still to install: Ghidra plus a Wii/GameCube loader (main decompiler), m2c (asm → C helper), and CMake + SDL3 for the PC build.
