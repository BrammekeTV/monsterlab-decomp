# Contributing

Help is welcome, whether you decompile by hand or with an AI coding agent (Claude Code, Codex,
Cursor, ...). This page is for people; [`AGENTS.md`](AGENTS.md) holds the instructions to give
your agent.

The goal is a **functional** decompilation: C++ that does what the original code does, so the
game can be ported to PC. The code does not have to compile back to identical PowerPC.

## Ground rules

- **Never commit game data.** No ISO, no `orig/`, no `build/`, no extracted assets, no asm
  dumps. They are git-ignored; keep it that way. Everyone uses their own copy of the game.
- Work on game code first (`0x80006B40`–`0x8022FEC4`). SDK, MSL, Bink and zlib only need to
  be identified, not decompiled.
- One pull request per batch of related functions. Keep PRs focused so they are easy to review.
- Don't edit `docs/data/` (progress site data; the maintainer updates it when merging) and
  don't hand-edit generated files such as `src/leaf/` (re-run `tools/gen_leaf.py` instead).

## Setup

1. **Game files.** Extract your own PAL disc (RM7P4F) so that `orig/RM7P4F/sys/main.dol` and
   `orig/RM7P4F/files/` exist (Dolphin: right-click the game → Properties → Filesystem →
   Extract Entire Disc). `main.dol` must have SHA-1 `50f6e9b94dbe4df6f663541493106d19e0a40c72`.
2. **Python 3.10+.**
3. **decomp-toolkit 1.9.1**: download `dtk` from
   <https://github.com/encounter/decomp-toolkit/releases> and put it at `tools/bin/dtk.exe`
   (`tools/bin/dtk` on Linux/macOS).
4. **Split the binary** into assembly (writes `build/RM7P4F/asm/`):
   ```bash
   tools/bin/dtk.exe dol split config/RM7P4F/config.yml build/RM7P4F
   ```
5. **m2c** (automatic C drafts):
   ```bash
   git clone https://github.com/matt-kempster/m2c tools/m2c
   ```
   ```bash
   python -m pip install "graphviz~=0.20.1"
   ```
6. **A compiler for checking your code.** On Windows install *Build Tools for Visual Studio
   2022* with the "Desktop development with C++" workload, then use `python tools/check.py`.
   Without it you can still contribute; GitHub CI compiles every pull request.

## Picking work

- `python tools/todo.py text` lists every function with its size and whether it is done.
  Big game functions are the most valuable.
- **Claim before you start**: open an issue (or a draft PR) titled `Claim: <function or area>`
  so two people don't decompile the same thing. Close it if you stop.
- Areas that are already well covered live in `src/game/` (see the file headers); extend those
  files rather than starting parallel ones.

## Workflow for one function

1. Read it: `sh tools/dump.sh <name>` (asm), `python tools/xref.py func <name>` (callers,
   strings), and the draft from `python tools/decomp.py <name>` (`build/m2c/<name>.c`).
2. Write it in `src/` with a `// 0xADDRESS` line directly above the definition, real types and
   field names, and a short comment on what it does.
3. Name things you understood in `config/RM7P4F/names_game.txt` (`fn_80012345 NewName`), then
   run `python tools/rename.py -f config/RM7P4F/names_game.txt` and re-run the dtk split.
4. Check: `python tools/check.py` (local compile), then `python tools/gen_leaf.py` so the
   generated leaf files drop anything you just decompiled.
   Then `python tools/check_protos.py` to catch a function declared with a different return
   type, parameter count or linkage in two files.
5. Commit and open a pull request. CI must be green.

## Pull requests

Describe in the PR what the functions do in game terms (what the player sees), list the
addresses, and mention anything you were unsure of. Reviewers compare against the asm, so
keep code readable: named fields and constants beat raw offsets where the meaning is known.
