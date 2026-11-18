# Brief: fix and verify m2c drafts (for subagents)

`python tools/draft_batch.py` already ran m2c over a batch and cleaned it into
`build/drafts/<name>.cpp` with a status per function in `build/drafts/<name>.report.txt`
(OK = compiles, FIX = compile errors / m2c leftovers, ASM = m2c could not handle it: tail
call, jump table, odd signature). A draft is only a starting point. m2c guesses types, drops
arguments, loses `u8`/`s16` widths and turns pointers into `s32`/`u32`.

Your job: turn the drafts named in your task into correct, repo-style functions in ONE new file
(named in your task, e.g. `src/game/draft_8019.cpp`), verified against the asm. Do not touch
other files except that file (others edit concurrently). No git commit, no config edits, no
scratch/dump files anywhere (read asm from `sh tools/dump.sh <name>` output only).

For EVERY function (status OK too, it is unverified):
1. `cd /f/monsterlab_redecomp && sh tools/dump.sh <name>`; read the draft in
   `build/drafts/<name>.cpp`. The asm is the truth: same branches, same order of side effects,
   same constants, same loads/stores widths (lbz/lhz/lwz/lfs ...).
2. Rewrite into the repo style: `// 0xADDR` ALONE on its line above the definition, definitions
   inside `extern "C" { }`, raw offset access `*(s32*)(p + 0x10)` (no `FLD` macro, no `vcall`
   helper: write virtual calls as `(*(Fn*)(*(u8**)obj + OFF))(obj, ...)`), keep original names,
   short comment on what it does in game terms. See `src/game/small_800E.cpp` for the style.
3. Types: pointers are `u8*`/`void*`, never `u32`/`s32` (the PC build is x64). Pointer
   slots in game data are read with `*(void**)` / `*(u8**)`. Floats `f32`, bytes `u8`, etc.
   from the load width. Script callbacks (names in `include/game/script_callbacks.h`):
   include that header inside `extern "C" { }` and match the signature exactly; callbacks
   returning pointers return `ScriptValue` (from `types.h`).
4. Callees: use an existing prototype if there is one (`grep -rn "<name>" include src | head -3`),
   else add `extern "C"` with a `// 0xADDR` comment and the types from the asm (`r3..r10`
   integers/pointers, `f1..` floats; return r3 / f1). A callee whose argument registers (r4..r10,
   f1..) are not set before the `bl` receives the *caller's own* leftover arguments: take and
   forward them (typical for constructors calling base constructors). A trailing `b X`
   (no `blr`) is a tail call: `return X(...)` with X's return type, never drop the value.
5. Constants/hashes: keep as named `static const u32 kHashX = 0x...u;  // "name"` if
   `python tools/hashes.py 0xHASH` resolves it. Do not invent names.
6. Skip a function rather than guess; list it in your report with the reason.

Check: `python tools/check.py <your file> 2>&1 | tail -30` (compiles, then runs
`tools/lint_decomp.py` and `tools/check_protos.py`). Fix every error and every lint/proto
finding for your file; a prototype conflict with another file means one of them is wrong, decide
from the asm and fix the other file's declaration only if it is a one-line change (say so).

Final reply (short): file path; done / skipped (with reason); check.py result; uncertain
functions. Never claim a function is verified unless you compared it with the asm.
