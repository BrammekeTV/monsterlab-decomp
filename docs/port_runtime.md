# PC port: runtime, globals and platform layer

Roadmap step 5, first part: the Windows build links into a real executable and runs the game's
own `main()` as far as the decompiled code reaches. This note says how the exe gets the
original's globals and data, how code that is not decompiled yet is handled, and what the
platform layer implements.

Code: `include/port/port.h`, `src/port/port_runtime.cpp` (memory, traps, scheduler, self-test),
`port_os.cpp` (OS, MEM), `port_sdk.cpp` (DVD, VI, the libraries that do nothing yet),
`port_msl.cpp`. Generator: `tools/port_link.py`. Local build: `python tools/build_pc.py`.

## Running it

```
python tools/build_pc.py                       # compile, generate, link build/pc/monsterlab.exe
build\pc\monsterlab.exe --selftest             # no game data needed (CI runs this)
build\pc\monsterlab.exe                        # runs main() with your own disc in orig/RM7P4F
```

| Option | |
|---|---|
| `--data <dir>` or `MONSTERLAB_DATA` | extracted disc (`sys/main.dol`, `files/`); default: `orig/RM7P4F` in the current folder or above the exe |
| `--frames N` | stop after N presented frames (default 300) |
| `--seconds N` | stop after N seconds and say where the main thread is (default 60) |
| `--keep-going` | a missing function returns 0 instead of stopping, and a semaphore nobody signals is passed after 0.5 s: a survey of what the boot path still needs. After the first one the run is no longer what the game does. |
| `--verbose` | log every skipped static initialiser and every SDK stub call |

The run ends with a boot summary: what was reached and when, disc files opened, SDK calls that
do nothing yet, and the stop reason. Exit codes: 0 frame limit or the game asked to exit,
2 no game data, 3 missing function, 4 crash, 5 time limit, 6 `OSFatal`.

CI builds the `monsterlab` CMake target (Release) and runs `--selftest`.

## Status

What boots today, with the retail PAL disc:

1. `main.dol` data sections loaded at their original addresses and byte-swapped.
2. The original's static initialisers (`.ctors`): those that are decompiled run, the rest are
   skipped and counted.
3. `SysInit`: `OSInit`, `SCInit`, `DVDInit`, the MEM1 OS heap and the MEM2 expanded heap,
   WPAD/KPAD/AI/AX init (stubs).
4. The engine heap (`HeapInit`, `fn_801EEB20`: Doug Lea's malloc) takes the MEM2 block and
   serves every allocation.
5. `GameMain`: `EngineStartup` (core and input init) and `GameInit` run to the end.
6. `EngineInit` -> `RendInit`, all of it: `VideoInit` picks the TV mode (EURGB60, 640x480 at
   60 Hz: the PC layer answers "60 Hz on, progressive off"), allocates the two frame buffers
   and the GX FIFO and starts VI and GX; the render thread starts and runs the first display
   list (its draw-done interrupt arrives); the 13 render targets ("hdrRed", "hdrGreen", ...)
   are allocated from the engine heap.
7. The rest of `EngineInit`: `SgInit`, the two built-in font textures (palette conversion and
   GX tiling), the first `RendBeginFrame`, and the layer set-up of `SetRenderEnabled`.
8. `GameStart`: the subsystem set-up (`fn_8000E488`), whose audio part runs on stubs. The disc
   queue opens **`catalog.sir`**, reads it to `0x90400000`, and the file layer converts it
   (6,394 sections in 59 files); the loader then opens all 59 `.sir` files.
9. `global_binary.sir`: header (4 KB) and image (1.1 MB to `0x90400000`) are read through
   `SirLoad` and converted in place (5,615 pointers, the game database, input table, textures,
   strings and actions).
10. Script system set-up (`fn_8010F794`): signatures and all script functions registered.
11. `UiInit` stops at `fn_8018C8F4` (232 bytes, not decompiled): the first stop today.

The link report (`port_link.txt`, uploaded by CI) lists the stubs and the globals.

## 1. Globals and initialised data

### The Wii address space is mapped at its original addresses

The runtime reserves `0x80000000`-`0x817FFFFF` (MEM1) and `0x90000000`-`0x93FFFFFF` (MEM2) with
`VirtualAlloc` before any other code runs. A 32-bit Windows process can use these addresses when
the exe is linked `/LARGEADDRESSAWARE` and runs on 64-bit Windows. Everything else follows from
this one decision:

- **A global the sources do not define is its original address.** `tools/port_link.py` writes
  an absolute symbol for it into `port_gen.obj` (`gGame` = `0x803B53A0`). No storage is
  declared and no table is needed at run time.
- **Access by raw address works** (`*(u32*)0x805FBBC0`), and so do the pointers stored inside
  the data: a vtable pointer in `.data` still points at the vtable.
- **The heaps hand out Wii addresses.** The arenas are the same ranges as on the console, so
  code that tests a pointer against `0x80000000`, or stores one in a 4-byte slot, behaves the
  same.
- **`.sir` images load where they were built for.** `GameStart` reads `global_binary.sir` to
  `0x90400000`; its internal pointers are valid as they are (byte-swapped), without relocation.

Two names for one address (`gSaveSystem` in one file, `lbl_805FBBE0` in another), or C linkage
in one file and C++ in another, resolve to the same place.

The cost: the exe is 32-bit, has a fixed base (`/FIXED /BASE:0x400000`) and needs 64-bit
Windows. That is no new limit: the decompiled code already needs 4-byte pointers. The 64-bit
build (roadmap step 6) replaces this scheme together with the raw offsets.

### Where the data comes from

No game data is in the repository or in the exe. At start-up the runtime reads the data sections
of the user's own `main.dol` (`.rodata`, `.data`, `.sdata`, `.sdata2`, `.ctors`) to their
addresses. `.bss` and `.sbss` are the zero-filled pages `VirtualAlloc` returns.

### Byte order

`main.dol` is big-endian. Each symbol's bytes are swapped by its layout; the table
(`gPortData`) is generated by `port_link.py` from, in this order:

1. `config/RM7P4F/port_data.txt` (optional overrides): `<name|0xADDR> <widths>`, the field
   widths in bytes, repeated over the symbol. `lbl_803A1234 4 4 2 2 1x4` is
   `{u32, u32, u16, u16, u8[4]}[]`.
2. The `data:` hint decomp-toolkit put in `symbols.txt` (`float`, `double`, `4byte`, `2byte`,
   `byte`, `string`): 6,900 symbols.
3. The type the sources declare the symbol with, when it is `u16`, `u32`, `f32`, a pointer or
   `f64`. A `u8`/`char` declaration is ignored: the decompiled code declares every untyped
   struct as a byte array.
4. Otherwise "auto", decided at load time from the user's data: printable text stays as it is;
   anything else that is 4-byte aligned is taken as 32-bit words (pointers, ints and floats,
   which is what CodeWarrior's data mostly is).

**Known gap.** Rule 4 is a guess. A struct that mixes `u16` or `u8` fields with words and has no
hint is swapped wrongly until it gets an entry in `port_data.txt`. About 1,300 symbols are
"auto"; most are vtables, jump tables and pointer tables, where the guess is right.

### Globals the sources define themselves

A global defined in a `.cpp` (`u8 gQuitRequested; ORIG(0x805FB758)`) lives in the exe, not at
its original address. That is fine as long as all code uses that definition. `port_link.py`
handles the two ways it goes wrong:

- **Another file uses the same address under another name** (`lbl_805FB758`). If the definition
  has external linkage, the other name is aliased to it, so there is one copy. Listed under
  "Names resolved to a definition under another name".
- **The definition is `static`**, so it cannot be aliased: the other name gets the original
  address and there are two copies. Listed under "Split globals" (0 today, and it has to stay
  0: the render thread's command queue was one of them). Fix: make the definition non-static,
  or drop it and declare the global `extern` in both files.

A defined global also does not get the initial value from `main.dol`; the definition must carry
it. `port_link.txt` lists the definitions without a value whose original lives in an
initialised section ("Defined in the sources without an initial value..."): each one starts as
0 on PC unless `main.dol` has 0 there too. The list must stay empty. (`gRendTexelsInMEM2` is
-1 in the original; as 0 it sent every texture allocation to a memory pool that does not exist.)

A defined array must also have the size of the original symbol (`symbols.txt`): the heap object
`gMainHeap` was defined with 0x40 bytes of its 0x728 and the allocator wrote over the globals
behind it. Nothing checks this yet.

## 2. Functions

`port_gen.obj` has one table entry per function of the original executable (12,587: address,
size, name, and the PC implementation if one is linked: 6,900 today).

- **Calls by name** link normally.
- **Calls through an original address** (vtables and callback tables in `main.dol` data, a
  function pointer stored by a static initialiser) land in the original code range
  `0x80004000`-`0x80345AE0`. The runtime fills it at start-up: at the address of every function
  with a PC implementation, a 5-byte jump to it; everywhere else `ud2`. No pointer in the data is
  patched, so nothing is misread as a pointer.
- **Functions that are not decompiled yet** (and SDK functions without a PC implementation) get
  a generated stub, `mov eax, <original address>; jmp eax`, which lands on a `ud2`. The
  exception handler looks the address up and reports name, address, size, region and the caller,
  then stops:

  ```
  [port] missing function: fn_8017A618 (0x8017A618, 1440 bytes, game code)
  [port]   called from fn_80182A1C+0x51
  ```

  Stubs are never written by hand. They are weak (`/alternatename`): when the function is
  decompiled, or the C runtime has it (`memcpy`, `strlen`), the real one wins and the stub is
  gone.
- A name the sources use that is not in `symbols.txt` (`HudInit`, `gFrameBudget`) is resolved
  from the address comment on its declaration (`// 0x8012B3A4`, `ORIG(...)`), or from the
  address in the name (`lbl_803C73EC`). If the function at that address is decompiled under
  another name, the old name is aliased to it.
- A function decompiled as `static` has no symbol: it cannot be reached through its original
  address. The report lists these; it matters only for functions that data points at.

One difference from the console: a function pointer taken in PC code is a PC address, the same
function reached through `main.dol` data is its original address. Code that *compares* function
pointers from both sources would see them differ. None has been found.

Crashes are reported the same way (exception, address, registers, call stack). Names come from
the linker's map file beside the exe (`monsterlab.map`), which includes static functions.

### Start-up order

| When | What |
|---|---|
| Before all C++ static initialisers (`init_seg(compiler)`) | map the address space, load and swap `main.dol`, fill the code range, install the handler; `--selftest` runs here and exits |
| After all of them (`.CRT$XCY`), before `main` | start the OS layer, then run the original's static initialisers from `.ctors` |
| `main` (the game's, `sys_main.cpp`, unchanged) | `SysInit`, `GameMain` |

The original's start-up code does the same between loading the executable and calling `main`.
It cannot wait for `OSInit`: static initialisers allocate memory, and the game's `operator new`
calls `SysInit` itself when the heaps do not exist yet.

A static initialiser that is not decompiled is skipped (`.ctors` has 300 entries; 37 run and
262 are skipped). The globals it would set stay zero. These are
missing game functions like any other, but nothing calls them by name, so they never show up in
the link report: the boot summary counts them and `--verbose` lists them.

## 3. Platform layer

An SDK function is defined in `src/port/` under the name the game code calls it by, with the
callers' parameter list: on PC that is the ABI. `fn_8024F1E8` is `DVDOpen`; the definition says
so in a comment. When an SDK function is renamed with `tools/rename.py`, the definition in
`src/port/` is renamed with it.

| Library | State |
|---|---|
| OS arenas, `OSInitAlloc` / `OSCreateHeap`, MEM expanded heap and allocators, MSL `malloc` | implemented: one block list per heap, inside the heap's own memory |
| OS time (`OSGetTime`, `OSGetTick`) | implemented at the console's timer rate |
| OS threads, semaphores, alarms | implemented on Windows threads under one lock (below) |
| `OSReport`, `OSFatal`, restart / menu / shutdown | console output; the last three end the run |
| DVD (`DVDOpen`, `DVDClose`, `DVDReadAsync`, drive status) | implemented on `<data>/files/`; the completion callback arrives from the interrupt thread |
| VI | retrace callbacks at the rate of the mode `VIConfigure` got (50 Hz for PAL, else 60), `VIWaitForRetrace`; a frame is counted at `VISetNextFrameBuffer` + `VIFlush` (the first one is `VideoInit` showing the empty frame buffer) |
| SC | fixed answers: 16:9, English, 60 Hz on, progressive off |
| GX | nothing is drawn. The state calls (about 80 so far: viewport, scissor, copy filter, matrices, blend, depth, vertex formats...) are recorded in `gPortGx` (`port.h`) for a later renderer. A display list handed to the graphics processor (`GXSetGPFifo`) is answered with the draw-done interrupt; its commands are not executed and draw-sync tokens are not delivered |
| MTX | `PSMTXIdentity` |
| Hardware registers | `0xCC000000`-`0xCC00FFFF` is plain memory: generated code writes the GX write-gather pipe (`0xCC008000`) and reads PI/VI registers by address. Writes are lost |
| AI, AX, WPAD, KPAD | stubs |
| MSL | `__register_global_object`, array construction helpers, `ceil_`; the standard C functions come from the Windows C runtime |

A stub that does nothing calls `PortStubCall`: the first call is logged and the boot summary
shows the count. An SDK function that has no definition at all stops the run like a missing game
function, unless it is in the short `PortSoftSdk` table in `port_sdk.cpp` (by original address,
with what it is): those log and return 0. Add to that table only after reading the asm.

### Threads and interrupts

The Wii has one core. Game threads never run at the same time, and interrupts (video retrace,
disc read completion, alarms) arrive between instructions. The decompiled code relies on that:
it has no locks of its own.

On PC every game thread is a Windows thread that runs only while it holds one lock, handed over
in arrival order. A thread gives the lock up only where it would block or poll: waiting on a
semaphore, and in `OSGetTime` (busy-wait loops call it). One more thread delivers the
interrupts, also under the lock, about once per millisecond. So an "interrupt" never preempts
game code in the middle of an update, which is stricter than the console.

A loop that waits for an interrupt to change memory without calling any SDK function would hang.
The time limit then reports where the main thread is.

### Disc files: conversion after the read

`DVDReadAsync` reads from the extracted disc and hands the bytes to `PortFileRead`
(`port_files.cpp`) before the game's callback runs. For a `.sir` file it finds the section the
read starts (by file name and offset, in a copy of the catalog's tables) and converts it in place
with `sir_loader.cpp`: the catalog itself when it is read, `global_binary.sir` when its image
arrives (the header section is kept until then: its pointers need the image). The game's
`SirLoad`, `SirFinishLoad` and catalog loader run unchanged. A section with no converter, a
packed one, or one read in pieces is left as it is and reported once.

The game polls `DVDGetDriveStatus` in a loop while a read is pending and relies on the drive's
interrupt to end it; on PC that call is where the interrupt thread gets to run.

## 4. Build

`port_link.py` reads the symbol tables of the compiled objects directly (COFF), so it needs no
failed link and no game data:

```
objects (game + src/port) --> port_link.py --> port_gen.obj + port_link.txt
objects + port_gen.obj    --> link /FIXED /BASE:0x400000 /LARGEADDRESSAWARE --> monsterlab.exe
```

`port_link.txt` is the link report: counts, address conflicts, aliases, split globals, static
functions, every stub (game and library) and the names left to the system libraries. A name in
that last list that is not a C runtime or Windows function is a typo or a symbol nobody gave an
address: the link fails on it.

The `monsterlab_link` target and `tools/missing_symbols.py` are unchanged: they still produce
the missing-symbols list for the progress site. That link fails by design.

MSVC only for now (absolute symbols, `/alternatename`, structured exception handling).

## Next: the shortest path to a first title-screen frame

1. **Follow the next stops** with `python tools/build_pc.py --run`: `fn_8018C8F4` in `UiInit`,
   then what a `--keep-going` run prints behind it.
2. **Converters for the other `.sir` sections.** The file layer (`src/port/port_files.cpp`)
   converts `catalog.sir` and `global_binary.sir`. Every other section the game loads is still
   big-endian: the boot summary's `data:` line counts the kinds it met, and the log names each
   one once (`disc: <file> section type N ... is not converted`). The title screen needs at
   least `shared_ui.sir` and `shared_texture.sir`. Packed sections are inflated by the game's
   own zlib (SDK range, not decompiled): link real zlib under those names, and convert after
   the inflate instead of after the read.
3. **Static initialisers**: 262 of the 300 `.ctors` entries are not decompiled. Most are small
   (they construct one global). Until they exist those globals are zero, which will show up as
   crashes or wrong behaviour further in. `monsterlab.exe --verbose` lists them.
4. **The GX command stream**: hand-written code writes vertices and register loads through
   `GXWGFifo`, generated code through `0xCC008000`, and both are lost today. A renderer needs
   them in order, in the FIFO the CPU is attached to (`GXSetCPUFifo`): make both one write-gather
   object that appends to the current FIFO, then execute a display list at `GXSetGPFifo`
   (which also delivers the draw-sync tokens `fn_8015D23C` dispatches).
5. **SDK names**: the functions `port_sdk.cpp` defines as `fn_8024xxxx` are identified in its
   comments (`VIConfigure`, `GXSetViewport`, ...). Renaming them with `tools/rename.py` needs
   the definitions in `src/port/` and the callers renamed in the same change.
6. **Initialised data**: add `port_data.txt` entries where a mixed struct is read wrongly (the
   seven `GXRenderModeObj` tables are in).
7. **GX**: draw (OpenGL). Until then a "frame" is only a VI flip.
