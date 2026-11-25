# Playthrough test

`python tools/playthrough.py` plays the first part of the game headless and checks that each step still happens.
`tools/smoke.py` only reaches the title and the main menu; this test covers the gameplay behind them.

## When to run it

Before every PR that changes gameplay: `src/game`, `src/engine`, `src/system`, `src/leaf`, `include/`, the loader or
runtime in `src/port`, or `config/RM7P4F/` (a rename can leave a function unresolved). Run `tools/smoke.py` first.

It is a local step, not a CI job: it needs the game data, which is never uploaded. It is a separate tool rather than a
`smoke.py` mode because it takes minutes instead of under one, and has its own stages and options.

```powershell
python tools/playthrough.py
```

The game is found as for any run (`orig/RM7P4F`, `MONSTERLAB_DATA` or `--iso PATH`). About 5 to 6 minutes, longer when
other copies of the game are running on the same machine.

## What it checks

First the link report: any game function linked as a stub (`build/pc/port_link.txt`) fails the test before the game
starts. Then one continuous run with `--to-menu --fast --no-audio --no-movies --no-ini --no-mods` and an empty save folder
(the exe is copied to `build/playthrough/ml_playthrough.exe`, so your own saves in `build/pc/save` are neither read nor
written). Markers come from `PORT_SCRIPT_TRACE=1` (script calls), `PORT_STATE_LOG=1` (`[state]` lines) and the level
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
| freemap | conversation closed, exploration HUD, a walk to the next node and arrival |

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

### Known hang on master

About one run in four stops during the first battle (seen at frames 7947, 8627 and 10160) with one core busy: the
render thread waits in `RingNext` (`src/port/port_gfx_d3d11.cpp`) for a Direct3D event query that never signals. Until
that is fixed, a stalled run is repeated once (`--retries`, only stalls; crashes and missed stages never are) and the
verdict says so, with the hung run's log in `build/playthrough/stall.log`. A hang in your change shows up as two stalls.

## Adding a stage

Append a `Stage` to `STAGES` in `tools/playthrough.py`:

```python
Stage("lab", "walk back to the castle", 23000,                  # key, text, deadline frame
      [call("SomeScriptFunction", 20500), level("castle_lab", 20500)],   # markers, any order
      press=("A@20600-20606",), pointer=("960,540@20550",)),
```

- Frame numbers count from the start of the run; `--press` / `--pointer` syntax is in `docs/port_runtime.md`.
  Pointer positions are pixels at 1920x1080.
- Markers: `call(name, after)`, `level(name, after)`, `state(label, regex, after)`. Give an `after` frame so that an
  earlier occurrence does not count. Many script functions (`target_node_selected`, `GetPerformFieldRepair`,
  `character_exploration_move_update`...) are polled every frame and prove nothing; check with `--check-log` on a run
  without your input that the marker does not appear.
- Leave a few hundred frames between the expected frame and the deadline. The timeline repeats closely but not to the
  frame (the battle intro camera's length varies, so the first attack lands at 10601 or 10901).
- A script function whose code is identical to another's is logged under one name for both.
- On the map, A is "interact": no stray A presses there.
