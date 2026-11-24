# Differential tests

`tools/difftest.py` checks decompiled functions against the original code: for one game function it runs
the original PowerPC instructions and our C++ on the same inputs and compares what they return and what
they write to memory.

```
python tools/difftest.py --fn <name|0xADDR>            one function
python tools/difftest.py --fn <name|0xADDR> --case K   one input in full: arguments, memory read, every difference
python tools/difftest.py --batch N                     N functions spread over the game
python tools/difftest.py --all                         every function with a // 0xADDR marker
```

| Option | Effect |
|---|---|
| `--calls` | also test functions that call other decompiled game functions (default: leaf functions only) |
| `--cases N` | inputs per function (default 40) |
| `--seed N` | another set of random inputs (default 1); a report line names seed and case, so every result can be repeated |
| `--audited`, `--unaudited` | only functions in / not in `docs/audit_log.txt` |
| `--globals` | some cases point global pointers that are still zero (`gBattle`, `gGameDb`, ...) at the test's buffers, found by scanning the function and its callees; the C++ must keep those globals at their original address, so a false MISMATCH can come from a global that does not |
| `--files S` | only functions whose source path contains S (comma-separated list) |
| `--no-leaf` | leave out the generated `src/leaf` functions |
| `--iso PATH` | the game disc (default: `orig/RM7P4F`) |
| `--no-build` | do not run `tools/build_pc.py` first |

It needs your own disc: the original instructions are read from its `main.dol` when the test runs.
Nothing of the game is stored: the report (`build/difftest/report.txt`, `report.json`) holds function
names, statuses and the differing values of failing cases, and `build/` is not committed.

## How it works

- **Where:** inside `build/pc/monsterlab.exe` (`--difftest JOBS`, written by the script). The PC build
  already has the game's data at the original addresses and every decompiled function linked, so both
  sides see the same globals. `src/port/port_difftest.cpp` holds a small PowerPC interpreter (integer,
  branch, load/store, float and the paired-single register saves of function prologues) and the harness.
- **Signatures:** `tools/difftest/sigs.py` reads each definition under a `// 0xADDR` marker and turns its
  types into argument kinds (integer widths, pointer, float, double). Integer arguments go to `r3`..., floats
  to `f1`...; the C++ function is called with the same values.
- **Inputs:** case 0 is all zeros, case 1 all ones, 2-5 edge values (`-1`, `0x7FFFFFFF`, `0x80000000`, ...)
  with empty memory, the rest random. Every pointer argument points into its own 8 KB buffer; the buffers
  are filled with small numbers, or with a mix of zeros, small numbers, floats, edge values and pointers to
  the other buffers. The simple cases come first, so the first failing case is usually the smallest one.
- **Running the original:** the interpreter may touch the test buffers, its own stack and the executable's
  data and bss. Writes are journalled and undone before the C++ runs. A read or write anywhere else (a
  wild pointer from random data, a NULL global) ends that case without a verdict.
- **Comparing:** the return value (at the width of the return type; floats within 1e-4 relative), every
  byte of the buffers, and every byte of the executable's data and bss (what the C++ changed against what
  the original changed). Float stores are compared as floats. A function pointer may be the console address
  on one side and the PC function on the other; a string constant may be the original's or the C++ literal.
- **Calls:** in a leaf run a call ends the test as "unsupported" (register save/restore helpers are
  followed). With `--calls` the interpreter follows calls into game functions that are decompiled, and the
  compiler's 64-bit helpers; the C++ side calls its own callees. A mismatch can then be in a callee.
  Library calls (`memset`, `strlen`, `cosf`, heap, SDK) are not followed yet.
- **Crashes:** if the C++ crashes, hangs or stops, the script charges it to the function that was running
  and goes on with the next one.

Memory is used in host byte order at the width of each access on both sides, which is what the port's data
conversion assumes everywhere. Float constants in `.sdata2` are read by the interpreter from the original
big-endian bytes, so a constant the port converts wrongly shows up as a mismatch.

## Statuses

| Status | Meaning |
|---|---|
| `MATCH` | at least one case was judged and all judged cases agree |
| `MISMATCH` | a case differs: the report shows arguments, the memory the original read, and each difference |
| `CRASH`, `HANG` | the C++ crashed or did not return on an input the original handled |
| `UNTESTED` | no case could be judged (every one left the test's memory); the reason of the first is given |
| `UNSUPPORTED` | not testable yet: calls something, a signature the harness cannot call, an instruction it lacks |
| `NOIMPL` | the definition is not linked into the exe (a `static` function, for example) |

A `MISMATCH` line ends with the command that repeats it:

```
MISMATCH fn_80065CB0 (0x80065CB0, src/game/small_8006.cpp):
    fn_80065CB0 case 0 seed ...: r3=buf0+0
      return: original 0x1, C++ 0x0
    reproduce: python tools/difftest.py --fn 0x80065CB0 --seed 1 --case 0
```

`buf2+16` is byte 16 of the third pointer argument's buffer; `in` lines are the non-zero values the
original read from the buffers, `out` lines the words that differ.

## Reading a mismatch

Check it against the asm (`sh tools/dump.sh <name>`) before changing code. Besides real decompile
mistakes (a comparison the wrong way round, a wrong constant, a missing store), these show up:

- **A global the C++ keeps elsewhere:** the original writes `0x805F....`, the C++ writes nothing there. The
  C++ has its own `static` copy, or declares the global without `ORIG(addr)`. Other code that reads the
  address sees a stale value: fix the declaration.
- **Mixed widths:** the original stores a byte and the C++ a 32-bit word with the same bits (or the
  reverse). Equal on the console, different bytes on PC. Use the width the original uses.
- **Undefined in C++:** shifts by 32 or more (zero on the console), float-to-int out of range. Cases the
  interpreter can recognise (division by zero, float-to-int overflow, NaN, huge and denormal floats loaded
  from memory) are skipped; the rest is reported.
- **Signature:** a source prototype that differs from the original's argument registers gives the two
  sides different inputs. That is a finding too, but in the prototype.

## Results (2026-11-24, seed 1, 40 cases)

8,550 marked functions (2,286 of them generated `src/leaf`).

| Run | Tested | Match | Mismatch | Crash | Untested | Unsupported |
|---|---|---|---|---|---|---|
| leaf functions | 3,663 | 3,621 | 29 | 13 | 845 | 4,037 |
| with `--calls` | 5,523 | 5,364 | 62 | 97 | 2,120 | 902 |

Of the functions in `docs/audit_log.txt` (886): leaf run 176 match, 1 mismatch, 2 crash; with `--calls`
350 match, 8 mismatch, 12 crash. The flagged functions are not all decompile mistakes: see "Reading a
mismatch"; with `--calls` most crashes are callees that need game state the test does not set up.

## Limits and next steps

- Globals are in the state after the static initialisers: no level, no heap objects. Functions that follow
  a global pointer are `UNTESTED` (1,877 with `--calls`). Next: let a test allocate what a global points at.
- Library callees are not followed (`memset`, `strlen`, `strcpy`, `vsprintf`, `cosf`: about 700 functions).
  Next: run those natively on both sides.
- No paired-single arithmetic, no byte-reversed loads, at most 8 integer and 8 float arguments, no structs
  by value, no member functions.
- A function that only writes through its own stack frame and returns nothing is compared on memory alone.

## Known harness limits

`tools/difftest/known_limits.txt` lists functions whose mismatch or crash is a limit of the harness (a static that lives at another
address in the PC build, host-order node references and u64 flags, a leftover `r3`, random input outside the real range). The report
counts them as `KNOWN` instead of `MISMATCH`/`CRASH`; `--show-known` lists them and `--strict` ignores the file. Add a line
(`0xADDR reason`) only after the asm shows the C++ is right.

`--globals --calls --files battle,attack_sequence,character_states,camera,anim` covers 119 of the 166 functions of those files
(111 without `--globals`), all matching; the rest need heap objects or library callees. Some random fills are mostly pointers or
half pointers, half small counts, for code that follows chains of records; buffers are 8 KB so records up to 7 KB can be tested.
