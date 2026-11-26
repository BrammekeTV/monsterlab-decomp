# Playthrough test

`python tools/playthrough.py` plays the first part of the game headless and checks that each step still happens.
`tools/smoke.py` only reaches the title and the main menu; this test covers the gameplay behind them.

## When to run it

Before every PR that changes gameplay: `src/game`, `src/engine`, `src/system`, `src/leaf`, `include/`, the loader or
runtime in `src/port`, or `config/RM7P4F/` (a rename can leave a function unresolved). Run `tools/smoke.py` first.

It is a local step, not a CI job: it needs the game data, which is never uploaded. It is a separate tool rather than a
`smoke.py` mode because it takes many minutes instead of about one, and has its own stages and options.

```powershell
python tools/playthrough.py
```

The game is found as for any run (`orig/RM7P4F`, `MONSTERLAB_DATA` or `--iso PATH`). About 14 minutes, longer when other copies of the game
are running on the same machine.
Leave the rendering in software: parallel runs on the graphics card can crash the display driver. `--gpu` is only for
problems that need the card.

## What it checks

First the link report: any game function linked as a stub (`build/pc/port_link.txt`) fails the test before the game
starts. Then one continuous run with `--to-menu --fast --no-audio --no-movies --no-ini --no-mods` and an empty save folder
(`MONSTERLAB_SAVE` points into `build/playthrough/`, so your own saves are neither read nor written; the exe is copied
there as `ml_playthrough.exe`, so that stopping other runs by the name `monsterlab.exe` does not end it). Rendering is
the headless default (WARP, the software rasteriser) at 640x360. Markers come from `PORT_SCRIPT_TRACE=1` (script calls), `PORT_STATE_LOG=1` (`[state]` lines) and the level
load messages, never from pictures.

| Stage | Passes when |
|---|---|
| boot | the splash screen script runs |
| menu | the menu level `castle_sea` is loaded and a screen is pushed |
| join | `StartQuest` and the Command Room level (`castle_communication`) |
| intro | briefing conversation, `village_level`, the coffin lands |
| mayor | Fuseless closes, the monster walks, the Mayor's conversation opens and closes |
| battle | `village_arena`, combat HUD, battle round 1 |
| attack | the first attack is chosen (`SetActionSelected`) |
| won | battle phase 8 (enemy defeated), results screen |
| rewards | rewards screen |
| repair | back in the village, `StartChallenge`, `LoadChallengeUI` |
| repaired | time-up, `DeinitChallenge`, health handed back |
| freemap | conversation closed, exploration HUD, a walk to the next node and arrival (the `map node` line of `PORT_STATE_LOG`) |
| battle2 | the walk up meets the second monster: `CombatPlaceholderEntry`, `village_arena`, combat HUD, battle round 1 |
| won2 | battle phase 8, results screen |
| awards | Combat Awards screen, `village_level` again, the next monster is spawned, exploration HUD |
| thanks | the Mayor's node, his conversation, `GotoCastleLocation`, the castle level, `HandleReturnToCastle`, the next `StartQuest` |
| name | Fuseless's lines in the castle, `EnterName_Init`: the scientist name entry screen (the first screen with a grid widget) |
| named | two letters typed on the keys (`EnterName_AddLetter`), the tick confirms: `EnterName_SaveNameToPlayerName`, `EnterName_Destroy`, Fuseless's welcome |

`python tools/playthrough.py --list` prints the markers and deadlines.

## Reading a failure

```
PLAYTHROUGH FAIL (180s): stage 'battle': not reached by frame 8100; waiting for 'load village_arena.sir'; last passed: mayor; log ends at frame 8101
```

The stages above the failed one passed. The last lines of the log without the script trace are printed below the
verdict; the whole log is `build/playthrough/run.log`. The other reasons are `crash at frame N` (the crash report is in the
log), `game stopped` (a `[port] STOP:` line: missing function, abort), `stalled` (no new frame for 90 s: a hang) and
`ended early` (the process vanished without a report: killed, or MSVC's stack check on a buffer overrun).

`--stop-after STAGE` ends the run after that stage while you work on it. `--check-log FILE` judges a saved log again,
which is the quick way to try new markers.

### The hang on the graphics card (worked around)

Before WARP became the headless default, about 4 of 12 runs stopped during the first battle (frames 7947, 8627, 10160)
with one core busy: `RingNext` (`src/port/port_gfx_d3d11.cpp`) waited without limit for a Direct3D event query that was
not reported. All of those runs shared an AMD card (RX 6800) with other running games.

The root cause is not established. The renderer's use of Direct3D was checked and no fault found: every use of the
device context during a run is serialised (`sGfxLock`), and a query is ended again only after it has signalled. With the
game alone on the card, 21 runs had no wait over 2 s. A driver problem under a shared card is the likely explanation, but
it was not reproduced: parallel runs on the card can crash the display driver.

The wait is now limited to 2 s. After that the buffer is mapped with DISCARD, which is safe whatever the GPU still
reads, and the run goes on. The log then has a line starting `gfx: no GPU mark for the`, with the query's result
(`0x00000001` = still not signalled) and the device state (`GetDeviceRemovedReason`, `0x00000000` = not removed). If you
see it, keep the log: those two values are what is missing to name the cause.

A stalled run therefore fails the test. `--retries N` still repeats one if you ask for it (the first run's log is then
in `build/playthrough/stall.log`).

## Adding a stage

Append a `Stage` to `STAGES` in `tools/playthrough.py`:

```python
Stage("lab", "walk back to the castle", 23000,                  # key, text, deadline frame
      [call("SomeScriptFunction", 20500), level("castle_lab", 20500)],   # markers, any order
      press=("A@20600-20606",), pointer=("960,540@20550",)),
```

- Frame numbers count from the start of the run; `--press` / `--pointer` syntax is in `docs/port_runtime.md`.
  Pointer positions are pixels of a 1920x1080 picture (the script scales them).
- Markers: `call(name, after)`, `level(name, after)`, `state(label, regex, after)`. Give an `after` frame so that an
  earlier occurrence does not count. Many script functions (`target_node_selected`, `GetPerformFieldRepair`,
  `character_exploration_move_update`...) are polled every frame and prove nothing; check with `--check-log` on a run
  without your input that the marker does not appear.
- Leave a few hundred frames between the expected frame and the deadline. The timeline repeats closely but not to the
  frame (the battle intro camera's length varies, so the first attack lands at 10601 or 10901).
- A script function whose code is identical to another's is logged under one name for both.
- On the map, A is "interact": no stray A presses there.
