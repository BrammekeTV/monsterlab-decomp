# Concept: the crashes in the difftest harness

Status: plan for review. No code has been changed.

"Crash" here means: in `tools/difftest.py`, the original PowerPC code finished a random test case inside the test's
memory, and our C++ crashed on the same input.

## Current list

`python tools/difftest.py --all --calls --globals --cases 160` on master 4e3150c (2026-11-26):
6,521 match, 108 known limits, **56 crashes** (`docs/difftest.md` recorded 49 on 2026-11-25; the difference was not analysed).

Group A is checked against the asm. Groups B to E are **likely** causes read from the crash site and the source.
Group F is not diagnosed.

| Group | Crashes | Cause | Possible in real play? |
|---|---|---|---|
| A. Float to unsigned (`fn_800222F4` and 8 callers) | 9 | **Real decompile bug.** The original calls `__cvt_fp2unsigned`, which gives 0 for a negative value. Our `(u32)` cast gives a large number, so we take the "record best time" branch and read through a null pointer. | Possibly, in minigame timers if end is before start. Not reproduced in play. |
| B. Badges and save counters | 20 | Badge flags are 64-bit words kept in host order (same as an existing known limit), so the two sides see different "already awarded" bits. The C++ then reads the random game database, partly inside the port-only badge log. | Unlikely: with a real database the ids are valid. |
| C. Globals the C++ keeps in its own storage (`GameSetNextMode`, `fn_8000B7A8`, `fn_801BA74C`, `PartQuery_AttackTargetsLimb`, `CharacterSetMode`) | 5 | `static ... ORIG(addr)` variables: the harness fills the original address, the C++ reads its own null copy. `CharacterSetMode` reads a `static const` table with all-zero flags; check that table against the disc. | No for the globals. The table: unknown. |
| D. Late zone ranges (`InLateZoneRanges`) | 5 | Reads the global settings block through a bad pointer. Why the original does not is unknown. | Unknown. Zone loading and waypoints use it. |
| E. Random function tables (`ScriptDispatch` x3, `EventPost`) | 4 | A random index or object gives a null function pointer. Same kind as existing known limits. | No. |
| F. Not diagnosed | 13 | See below. | Unknown. |

Group F: `fn_8003B834` (call to null), `fn_8006A2D0` (damage), `WorldOnZoneLoaded`, `InterpolateTo`, `fn_801BA930`,
`fn_801BC574`, `fn_801DA7C8`, `ZoneIsLoaded` (via `fn_801E2470`), `fn_801EDA4C`, `fn_801EDF58`,
`MeshStartBouncingOverlayAlpha` (in `PoolReclaim`), `LabReleaseMonsterAt`, `RemovePlayerMonster`.
`fn_801DA7C8` matches its asm, so it is probably overlapping random buffers. The zone, lab and effect-pool functions
run in normal play, so triage those first.

## How to find a real cause

1. Repeat one case: `python tools/difftest.py --fn <addr> --case K --calls --globals --cases 160`.
2. Read `sh tools/dump.sh <name>` next to the C++. Check, in this order: float to integer conversions, `cror`
   comparisons, field widths (byte or word), dropped arguments, big-endian fields not converted, and globals
   without `ORIG` or declared `static`.
3. **Gap in the tool:** a crashing case prints only the C++ crash address. The original's inputs, memory reads and path
   are lost, which is why groups D and F are open. Print them before the C++ runs (task 2).
4. For a function that really runs in play, confirm in the game: add a log and run `python tools/playthrough.py`.

A mistake gets a code fix. A harness limit gets a line in `tools/difftest/known_limits.txt`, only after the asm shows the C++ is right.

## Fix plan, ranked (one PR each)

1. **Fix the float-to-unsigned bug** (9 crashes, small). Then check the other `__cvt_fp2unsigned` call sites (90, in 64 functions) for
   the same plain cast. #200 and #260 already fixed this mistake one function at a time.
2. **Better crash reports:** arguments, the original's reads and the C++ fault address for a crashing case (small).
   All later triage depends on this.
3. **Harness: static globals.** Do not run a case that relies on filling a `static ... ORIG` address, or copy the value
   into the C++ variable. Check the `CharacterSetMode` table in the same PR (small to medium; 5 crashes).
4. **Harness: badge flags and port-only logging.** Give both sides the same flag bits and silence the badge log under
   difftest. Re-run and check what is left of the 20 (medium).
5. **Diagnose group D and the zone, lab and pool functions of group F** with the task 2 output (medium; about 10 crashes).
6. **The rest, and known-limit entries** for group E and confirmed noise (small).

Done means: the full run reports 0 crashes, with every remaining case listed in `known_limits.txt` with its reason.
Verify each PR with the full difftest run, `tools/smoke.py` and, for game code, `tools/playthrough.py`.
