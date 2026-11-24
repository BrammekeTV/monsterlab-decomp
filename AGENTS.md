# Instructions for AI agents

You are helping decompile **Monster Lab** (Wii, PAL `RM7P4F`) into readable, functional C++ for
a PC port. Read this whole file before making changes. Human contributors: see
[`CONTRIBUTING.md`](CONTRIBUTING.md) for setup.

## Hard rules

1. **Never commit or publish game data**: nothing from `orig/`, `build/`, the ISO, extracted
   assets, or asm listings. Don't paste large asm or data dumps into issues or PRs either.
2. **Functional, not matching.** The C++ must behave exactly like the PowerPC code (same
   branches, same order of side effects, same constants), but it does not have to recompile to
   identical machine code.
3. **Verify against the asm.** The m2c draft is a hint, not the truth. Every function you
   write must be checked against `sh tools/dump.sh <name>` before you commit it.
4. **Don't invent names.** Name things only from evidence: script callback names
   (`tools/script_funcs.py`), strings the code uses, debug messages, hashes resolved with
   `tools/hashes.py`, or behaviour you have actually traced. If unsure, keep `fn_XXXXXXXX` /
   `unkXX` and describe what you saw in a comment.
5. **Don't hand-edit generated files**: `src/leaf/*.cpp` (from `tools/gen_leaf.py`),
   `src/game/script_registry.cpp`, `script_signatures.cpp`, `script_dispatch.cpp`,
   `entity_factory.cpp` and `include/game/script_callbacks.h` (from the `tools/gen_*.py`
   scripts). Don't edit `docs/data/`; the maintainer updates the progress site.
6. Work only in the game range `0x80006B40`–`0x8022FEC4` unless asked. SDK/MSL/Bink/zlib code
   above it is to be identified, not decompiled.
7. Before you start a function, check that nobody claimed it (open issues/PRs titled
   `Claim: ...`) and that it is not done: `python tools/todo.py text | grep <addr>` shows
   `DONE` for decompiled functions.

## Repository map

| Path | What |
|---|---|
| `config/RM7P4F/symbols.txt`, `splits.txt` | dtk symbols/splits; changed only through `tools/rename.py` |
| `config/RM7P4F/names_*.txt` | record of every name given (`old new`), by source |
| `build/RM7P4F/asm/*.s` | asm from `dtk dol split` (local only) |
| `include/` | shared headers: `types.h`, `revolution.h` (SDK decls), `game/*.h`, `engine/*.h` |
| `src/game/`, `src/engine/`, `src/system/` | hand-decompiled code, one file per system |
| `src/leaf/` | generated tiny functions |
| `src/port/`, `include/port/`, `tests/` | PC port code that has no original: data loader (`docs/loader.md`), runtime and platform layer that replaces the SDK (`docs/port_runtime.md`), and tests; no `// 0xADDR` markers here |
| `tools/` | scripts (below) |

## Tools

- `sh tools/dump.sh <name>`: compact asm of a function.
- `python tools/decomp.py <name|0xADDR>`: m2c draft in `build/m2c/<name>.c`, with known struct
  types (`tools/m2c_context.h`) and every `extern "C"` prototype from the sources.
- `python tools/m2c_fifo.py build/m2c/<name>.c`: the m2c draft with the widths of its GX pipe writes
  (`*(s8 *)0xCC008000 = x` becomes `GXWGFifo._u8/_u16/_u32/_f32 = x`, from the command stream).
- `python tools/xref.py func <name>`: callers, callees, strings. `xref.py strings <regex>`.
- `python tools/todo.py <object substring>`: functions, size (instructions), DONE marker.
- `python tools/hashes.py <0xHASH>`: the string behind a `StringHash` (CRC-32 of the upper-cased
  name). Run `python tools/hashes.py build` once.
- `python tools/rename.py -f config/RM7P4F/names_game.txt`: apply names, then re-run
  `tools/bin/dtk.exe dol split config/RM7P4F/config.yml build/RM7P4F` so asm and m2c see them.
- `python tools/gen_leaf.py`: regenerate `src/leaf/` (drops functions that are now done).
- `python tools/draft_batch.py --size 0x80 0x300 --region 0x8019 --count 15 --name draft_8019`: runs m2c
  over a batch of undecompiled functions, cleans the output into compilable C++ and reports per function
  OK / FIX / ASM in `build/drafts/` (staging, not committed). Hand the drafts to a subagent with
  `tools/draft_fix_brief.md`; every function is still verified against the asm before it goes into `src/`.
- `python tools/undone.py [--min N] [--max N] [--count 60] [--batch 20] [--missing-first] [--renderer]`: lists game functions with no
  `// 0xADDR` marker (generated `src/leaf` wrappers and markers in open PRs excluded), largest first, in batches for `draft_batch.py --names`.
- `python tools/lint_decomp.py [files|--all]`: flags pointer-truncating casts, dropped tail-call return values,
  base-constructor argument pass-through, `// 0xADDR` markers not alone on their line and scratch files
  (file:line; `lint:ok` comment suppresses an intentional one). `tools/check.py` runs it after compiling.
- `python tools/check_protos.py [names]`: reports functions declared with different return types,
  parameter counts or C/C++ linkage across `src/` and `include/` (run it before pushing; all game
  functions use C linkage). Pointer-returning script callbacks return `ScriptValue`.
- `python tools/link_check.py` then `python tools/missing_symbols.py`: compile (Win32) and link every object;
  the linker output becomes the to-do list: `docs/data/missing.json` plus a summary of unresolved game
  functions (most-referenced first, with address and size), library functions by SDK region, globals,
  symbols unknown to `symbols.txt`, duplicate definitions and **mismatches** (a symbol defined with another
  type or C/C++ linkage: fix these, they are never missing code). CI runs the same after the compile and
  fails only on duplicates/mismatches. Declare globals with the type of their definition; `tools/gen_leaf.py`
  copies those types into `src/leaf/`.
- `python tools/check.py [files]`: compile with MSVC using the CI flags (no args = changed files).
- `python tools/loader_check.py`: builds the PC data loader test, runs its self-test and, with the game in `orig/`,
  compares the loaded `global_binary.sir` with `python tools/sir_decoder.py <file> --gamedb --image` (the reference dump).
- `python tools/build_pc.py [--run [args]]`: builds `build/pc/monsterlab.exe` (compile, `tools/port_link.py`, link). `--run --selftest`
  needs no game data; `--run` alone runs the game's `main()` with the disc in `orig/` until the first function that is
  not decompiled and prints a boot summary; `--run --keep-going` lists every missing function on the way instead.
  `build/pc/port_link.txt` is the link report: stubs, globals resolved by address, **split globals** (a `static`
  definition in one file and the same address used under another name elsewhere: two copies, fix it) and address
  conflicts. See `docs/port_runtime.md`.
- `python tools/smoke.py [--iso PATH]`: **required before every PR that touches `src/` or `include/`** (port and game code). Builds incrementally,
  then runs headless: `--selftest`, `--to-menu` to frame 3300 and the title at frame 2600. Passes when each run reaches its frame limit
  without a crash, the menu scripts ran (`UIScreen_Push`) and neither screenshot is blank or black (`build/smoke/*.png`). One verdict line
  with timings (about 50 s); `SMOKE FAIL` lists the reasons and keeps the logs in `build/smoke/`. Needs the game data (`orig/RM7P4F` or `--iso`).
- `python tools/mod_test.py [--iso PATH] [--keep]`: texture-mod test (`docs/modding.md`): dumps the menu's textures to a temporary folder outside
  the repository, makes a mod with one tinted red, runs headless again and compares the screenshots. Run it after changes to
  `src/port/port_mods.cpp`, `port_png.cpp` or the texture cache in `port_gfx_d3d11.cpp`.
- `python tools/difftest.py --fn <name|0xADDR>` / `--batch N` / `--all [--calls]`: differential test (`docs/difftest.md`): runs the original
  PowerPC code of a function (from your disc, in an interpreter inside the exe) and the C++ on the same inputs and compares return value
  and memory. Run `--fn` on a function after decompiling or changing it; a `MISMATCH` prints the command that repeats the failing case.
- `python tools/bink_check.py [file.bik]`: builds the PC Bink decoder (`src/port/port_bink_codec.cpp`, derived from FFmpeg, LGPL: `third_party/bink/`) and compares
  frames and audio of a movie from your disc with `ffmpeg.exe` (reference only, never shipped). Run it after changing the decoder.
- `python tools/progress.py`: progress numbers.
- `python tools/changelog_add.py "text"`: add a progress-site changelog entry stamped with the real time (maintainer only).

## How to decompile a function

1. `sh tools/dump.sh fn_X > build/fn_X.s` and `python tools/decomp.py fn_X`.
2. Find out what it is: callers (`xref.py`), strings, hashes, the script name if it is a
   registered callback (signature in `include/game/script_callbacks.h`).
3. Write it in the right `src/` file (or a new file for a new system, with a header comment that
   names the system and the original address range). Put `// 0xADDRESS` alone on the line
   directly above the definition; `tools/progress.py` counts these markers.
4. Declare callees you don't decompile in an `extern "C" { ... }` block at the top of the file
   with a comment giving their address, e.g. `void PartSetTier(PartInstance* p, u8 tier); // 0x8021DA68`.
   Use names from `symbols.txt` (after renaming) so other files and m2c pick them up.
5. Globals: `extern` them with the symbol name and their address in a comment, or define them in
   the file that owns them with `ORIG(0x...)` after the declaration.
6. Add the names you found to `config/RM7P4F/names_game.txt`, run `tools/rename.py`, re-split.
7. `python tools/gen_leaf.py`, then `python tools/check.py`. Fix every error.
8. Commit with a message listing the functions; open a PR describing behaviour in game terms.

## Reading the asm (CodeWarrior / PowerPC)

- Arguments: `r3`–`r10` (integers/pointers), `f1`–`f8` (floats); return in `r3` / `f1`.
  `r13` is the small-data base (`@sda21` symbols), `r2` the second small-data base.
- Virtual calls: `lwz r12, 0(rX)` (vtable), `lwz r12, OFF(r12)`, `mtctr r12`, `bctrl`. Write them
  with the vtable byte offset, e.g. `(*(Fn*)(*(u8**)obj + 0x14))(obj)`, as the existing code does.
  UI elements have a second vtable at `+0x138` (slot `0x2C` show, `0x30` hide).
- `cror eq, lt, eq` / `cror eq, gt, eq` after `fcmpo` means `<=` / `>=`. m2c prints these as
  `M2C_ERROR` and often shows `==`: fix the comparison.
- `cntlzw rD, rS; srwi rD, rD, 5` is `rS == 0`. `rlwinm`/`extrwi` extract bit fields;
  `extrwi rD, rS, 1, N` is `(rS >> (31 - N)) & 1`.
- `lis rX, A; addi rX, rX, B` builds 32-bit constants (often `StringHash` values); `subi`/`addi`
  with a negative low half are common. Double-check the arithmetic.
- `frsqrte` followed by two Newton steps is the engine's `1/sqrt`; write `1.0f / sqrtf(x)`.
- `xoris r0, r, 0x8000` + `lfd` + `fsubs` with the `0x4330...` double is an int-to-float
  conversion; `fctiwz` + `stfd` + `lwz` is float-to-int truncation.
- `_savegpr_N` / `_restgpr_N` are register save helpers, not real calls.

## Common m2c mistakes to fix

- Missing arguments: m2c sometimes drops `r3`/`r4` it thinks are dead (e.g. a widget passed in
  `r3` to a UI call). Check every call's argument registers in the asm.
- Struct copies (`lwz`/`stw` sequences to the stack) appear as dozens of `spXX` variables; they
  are usually one struct assignment.
- Loops over fixed arrays may read one element past the end after the loop (the original does
  that too; keep the behaviour and comment it).
- `switch (1)` / `switch (0)` means an inlined helper was called with a constant; restore the
  helper.
- Float arguments passed to a call that m2c lists with 10+ `f32` parameters: only the first
  ones are real.

## Conventions

- C++17, MSVC and GCC/Clang. Use `types.h` (`u8`, `s32`, `f32`, `BOOL`, ...).
- Pointers in game data are 32-bit on the Wii, so the PC build is **32-bit (Win32)** for now (`types.h` asserts it; x64 comes after named structs and a loader). Access raw
  fields with `*(T*)(p + 0xOFF)` until a struct is known; define structs in `include/` once a
  layout is solid and reused.
- Comment *what* the code does in game terms (what the player experiences), not how the asm did it.
- Keep generated-looking noise out: one temp per meaningful value, real loops, named constants
  for hashes and magic numbers you understand.
- Commit messages: `Decompile <Name>, <Name> (...)`. Don't add attribution you were not asked for.

## Useful facts

- `StringHash` (0x80347E68 table) is CRC-32 of the upper-cased string; UI widgets, events,
  animations, sounds and NIS signals are all addressed by these hashes.
- Script callbacks are registered by name in `RegisterScriptFunc` (`src/game/script_registry.cpp`);
  their signatures are in `include/game/script_callbacks.h`. These names are the most reliable
  original names in the game.
- `gGameDb` (global_binary.sir) holds part definitions, actions, abilities and tuning
  (see `include/game/part.h` and the README's `.sir` notes).
- The world (`gWorldState`, 0x803E6D00) owns zones, zone slots and entities: see
  `src/game/world_update.cpp` and `world_alloc.cpp`.
