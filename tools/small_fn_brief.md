# Brief: small-function batches (for low-cost agents)

You write equivalent C++ (compiles on Windows MSVC, functional not matching) for a given
list of small PowerPC functions, into ONE NEW file named in your task.

- Read each function's asm: `cd /f/monsterlab_redecomp && sh tools/dump.sh <name>`.
  Don't use other tools on build/ or orig/.
- Style: `head -60 src/game/small_800E.cpp`; script callbacks: see how
  `src/game/small_800F.cpp` includes `include/game/script_callbacks.h` (match its
  signatures exactly). `#include "types.h"`; raw access `*(s32*)(p + 0x10)`; definitions
  inside `extern "C" { }`.
- For every callee/global: `grep -rn "<name>" include src | head -3`. Header declares it →
  include the header and match exactly. Declared in another .cpp → copy that prototype.
  A function in your list already DEFINED elsewhere → skip it. Otherwise add an
  `extern "C"` prototype with a `// 0xADDR` comment. File-scope helpers must be `static`.
- Above each function: `// 0xADDR` (uppercase hex) + optional one-line description.
  Keep original names.
- PowerPC: args r3..r10 / f1..f8, return r3/f1. `bl X` = call. Trailing `b X` = tail call:
  `return X(...)` with a matching return type. If a call (e.g. base constructor) happens
  without r4..r10/f1.. being set first, those args pass through: take and forward them.
  `lwz rN, name@sda21(r0)` = global. Virtual call `lwz r12,0(r3); lwz r12,OFF(r12);
  mtctr r12; bctrl` = `(*(Fn*)(*(u8**)obj + OFF))(obj, ...)`. Deleting destructor: base
  dtor with -1, `operator_delete(p)` if del > 0, return p. Store pointers (vtables, tables)
  as `*(void**)(p + off) = ptr;`, never via `(u32)` casts (the PC build is x64). `fcmpo` + `cror eq,gt,eq` = `>=`,
  `cror eq,lt,eq` = `<=`. Float constants may be `extern f32 lbl_X;`.
- Skip anything unclear rather than guess. Do not write any file other than your own
  new source file (no scratch/dump files anywhere); read asm straight from tools/dump.sh output.
- `python tools/check.py 2>&1 | tail -15`: fix errors in YOUR file only (others edit
  concurrently). Edit only your file. No git commit, no config/ edits, no dtk/rename/progress.

Final reply: file path, done/skipped, compiled or not, uncertain functions. Short.
