# Savestates

A savestate is the whole running game in one file. A headless test loads it and goes on from the middle of
the game instead of replaying from boot. Code: `src/port/port_state.cpp`, the scheduler in
`src/port/port_runtime.cpp`, the thread stacks in `src/port/port_os.cpp`.

```
build\pc\monsterlab.exe --fast ... --save-state village.mlst@5200 --save-state battle.mlst@9000
build\pc\monsterlab.exe --fast ... --load-state village.mlst --frames 5400
```

| Option | |
|---|---|
| `--save-state FILE@FRAME` | write the state at the end of frame FRAME (as `PortFrameDone` counts them); up to 128 per run |
| `--load-state FILE` | start from that state instead of booting. Frame numbers go on from the state's: `--frames`, `--press`, `--pointer`, `--shot-at` and `--save-state` mean the same frames as in the run that saved |

Give a run that loads a state the same input options as the run that saved it (the same `--press` and
`--pointer` list): input is by frame number and is not part of the state. The tests do this by passing the
whole list every time.

## A state belongs to one exe

**A state loads only into the exe that wrote it.** The file holds the game threads' stacks and the statics
of the exe, and both are full of code addresses of that build. The header has the CRC-32 of the exe file
and of `main.dol`; another build or other game data is refused with a message that says which build made
the state and to make the states again. So:

- after every rebuild the states are stale: `tools/playthrough.py --from` says so, and one full run
  (`--refresh`) makes them again;
- within one build a state saves the replay every time: trying input for a new playthrough stage, repeating
  a test, looking at one moment with different debug options (`PORT_SOUND_LOG`, `--screenshot`, `--peek`...).

States that survive a rebuild would need the stacks and every function pointer in the game's memory to be
independent of where the linker put the code. That is not done (see "Not done").

## What is in the file

| Part | How |
|---|---|
| MEM1, MEM2, the hardware register page | whole, in 4 MB pieces (XPRESS-Huffman from Windows' compression API; an all-zero piece takes no space) |
| The game threads' stacks | the main thread and every `OSCreateThread` thread run on stacks at fixed addresses (`PORT_STACK_BASE`, 0xA0000000, 5 MB apart); the used part of each is saved, with the registers of the place where the thread waits |
| The game code's statics | every static of a source that does not include `port/port.h` goes into the exe sections `.savd`/`.savb` (`types.h` sets them); both are saved whole. Globals with `ORIG(...)` live in MEM1 anyway |
| The emulated console | the port's statics between `PORT_SAVED_BEGIN` and `PORT_SAVED_END` are in the same sections: OS heaps, arenas, alarms, thread table, the game clock, the scheduler's queues, queued interrupts, open disc files, VI counters and callbacks, the GX register files (BP, CP, XF) and FIFO state, the SDK's GX shadow state, AX voices, MIX and AXART state, the last controller sample, the frame counter, the `[state]` log's last values, the script-trace table |
| The port's fixed heap | `PortHostAlloc` (0xB0000000): the catalog copy, the loader's lists of what it converted in a level (`SirLoaderSetAllocator`), level-load bookkeeping and zlib streams, so a level that loads after the state, or is loading while it is taken, converts as in the straight run |
| Named blocks | the renderer's palette memory (`PortStateBlob`) |
| The renderer's pictures | the frame buffer and every frame-buffer copy, read back from the graphics device as RGBA (`PortGfxStateSave`): the next frame is made from them |
| The save folder | `game.msa`, `banner.bin` and `tmp\*`, as files. They are written back only into a `MONSTERLAB_SAVE` folder (tests); a player's own save folder is never touched by a load |

Not in the file, made again after a load: textures, shaders and vertex buffers of the renderer (decoded
again when first used); the depth buffer (the game clears it every frame); the audio effects' delay lines
(reverb, chorus and delay start from silence: sound only, the game does not read them); the sound device; the
window. The pictures are at the saving run's render size: a run that loads at another `--res` or `--msaa` says
so and draws its first frames without them.

Sizes: about 10 MB at the main menu, 22 to 27 MB in a level (at 640x360; the pictures grow with the render size).

Not written, with a log line saying why: while a Bink movie plays (use `--no-movies`), while the game has a
save file open, and at a frame that ends inside an interrupt of a paced run (save in `--fast` runs).

## How a load works

The process starts as usual (memory mapped, `main.dol` loaded, renderer started). Instead of running the
game's static initialisers and `main()`, `PortStateLoad` checks the header, puts the memory, stacks, sections
and blocks back, reopens the open disc files, and starts one Windows thread per saved game thread. Each
switches to its saved stack and continues at the place where the saved thread waited for the lock
(`Block` in `port_runtime.cpp`). The main thread continues inside `PortStateFrame`, where the saving run was
when it wrote the file, and returns from there into the game's frame loop.

Three things make that possible:

- **Fixed stacks.** A game thread's Windows thread switches to its slot's stack (`PortStackEnter`) and sets
  the thread's stack bounds and exception chain to match, as a fiber switch does. The exe's entry point
  (`PortEntry`) sets MSVC's buffer-overrun cookie to a constant before the C runtime starts, because the
  cookie is in every checked stack frame.
- **Statics by section.** Port sources are host state by default; what belongs to the emulated console is
  marked. Game sources are saved by default.
- **A scheduler that is data.** See the next section.

### Rules for port code

- A static in `src/port/` that the game's behaviour depends on belongs between `PORT_SAVED_BEGIN` and
  `PORT_SAVED_END` (file scope; function-local statics are hoisted). Handles, `FILE*`, C-heap pointers,
  options and anything read from the environment stay outside (`PORT_HOST_BEGIN`/`END` inside a saved
  region).
- Port buffers the game's state points into come from `PortHostAlloc`, not `malloc`.
- What must be made again in the new process (an open file) registers a function with `PortStateOnLoad`.
- Host memory outside the sections registers with `PortStateBlob`.
- `kVersion` in `port_state.cpp` changes when the file layout changes.

## The scheduler, and why unpaced runs now repeat

Before, the lock between game threads was a ticket lock on Windows primitives, and a thread of its own
delivered the "interrupts" (retrace, disc read done, alarms, AX frames) about once a millisecond of real
time. Which thread got the lock next, and at which poll of a game thread an interrupt arrived, depended on
real time. Two `--fast` runs with the same input drifted apart: `[state]` lines at different frames after a
few thousand frames, fights of different lengths.

Now the lock is handed on through a queue that only the lock holder changes. In an unpaced run (`--fast`,
`--fast-forward`) nothing else takes part: the running thread delivers what is due itself where it polls
(`OSGetTime`, `DVDGetDriveStatus`, a semaphore signal: `PortYield`), and when every thread waits, the last
one to wait moves the game clock to the next event and delivers it (`PortWaitChange`). The interrupt thread
only works in paced runs.

Two more sources of difference between runs were removed:

- `rand()`/`srand()` were the C runtime's, which keeps one state per thread and outside the game's memory.
  They are now the original's (one generator, state at 0x805FAE38, `port_msl.cpp`).
- The game seeds it from `time()`. In an unpaced run `time()` is a fixed date plus the game clock.

Paced runs (a window, real time) are as before: real time decides when the interrupt thread gets in.

## Exactness

`python tools/savestate_test.py` does this (run it after changing the state code): one straight run that saves a
state at each point N and takes a screenshot at N + 200; then, per point, a new process that loads the state and
runs to N + 200. Compared: every `[state]` and `[script]` log line of the frames between, and the screenshot
(640x360, software rasteriser), byte for byte.

Results (the playthrough's input, `PORT_SCRIPT_TRACE=1 PORT_STATE_LOG=1`):

| Point | Saved at | Compared at | Log lines | Picture | State file |
|---|---|---|---|---|---|
| main menu | 3200 | 3400 | 1001, identical | identical | 10.0 MB |
| village map | 5200 | 5400 | 1868, identical | identical | 22.1 MB |
| walking into the first enemy (the arena level loads after the state) | 7500 | 7700 | 1831, identical | identical | 22.1 MB |
| first battle, mid-fight | 10000 | 10200 | 1607, identical | identical | 26.5 MB |
| rewards screen (the village level loads after the state) | 16400 | 16600 | 1540, identical | identical | 26.7 MB |
| castle, Fuseless talking before the name entry | 29000 | 29200 | 662, identical | identical | 24.1 MB |

The playthrough itself is the longer check: `--from name` (state of frame 28500) passes the name entry stage at frame
29035, the frame the full run passes it at, in half a minute instead of 18.

There is no rendering noise to allow for: two straight runs of the same exe are identical too (checked to frame
5400: all 15,531 log lines, and the pictures at 3300 and 5400 byte for byte; three full playthroughs passed all 17
stages at the same frames). Before the frame buffer and its
copies were put into the state, a loaded run's picture differed from the straight run's in about 1 % of the
bytes by up to 3 levels, 200 frames later: the engine's feedback effects read the last frame's copies, and 8-bit
feedback never quite forgets. That is why they are in the file.

Also checked by hand: a paced (real-time) headless run saves and a paced run loads; a state of another build and a
file that is no state are refused with a message.

## Not done

- **States that survive a rebuild.** It would take: function pointers in game memory that do not depend on
  the link (the code range at 0x80004000 already has a jump at every function's original address;
  `tools/port_link.py` could point address-taken functions there), game statics and string literals at stable
  addresses, and threads that restart from a known point instead of a saved stack.
- The audio effects' delay lines and the depth buffer are not in the file (see above).
- Loading into a window (`--window --load-state`) is written (the clock goes on from the state's game time
  in real time) but was not tested: the tests run headless.
