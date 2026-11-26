# Concept: a steady 60 fps at 1080p

Status: plan for review. No code has been changed.

## Measurements (2026-11-26, master 4e3150c)

PC: Ryzen 7 5700X, Radeon RX 6800. Release build, headless on the GPU, 1920x1080, no mods or audio.
**Other sessions kept the CPU at 70-100% during the runs.** Treat every figure as pessimistic and about ±20%.
Times are ms per frame. The 60 fps budget is 16.7 ms.

| Scene | avg | fps | p95 | p99 | worst | render | GX draws | vertices |
|---|---|---|---|---|---|---|---|---|
| Main menu (`--bench menu`) | 16.7 | 60 | 18.3 | 19.2 | 21 | 6.4 | 7.8k | 79k |
| Castle, Command Room (frames 3650-4250) | 17.5 | 57 | 22.4 | 27.1 | **138** | 7.5 | 7.7k | 82k |
| Castle, Grand Hall to Mech Lab (33300-33900) | 19.3 | 52 | 25.2 | 33.4 | **143** | 10.6 | 11.7k | 113k |
| Village map (`--bench village`, 4 runs) | 21.3-25.0 | 40-47 | 24-31 | 26-35 | 33-43 | 12.9-14.6 | 19.3k | 160k |
| First battle (8300-9200) | 24.3 | 41 | 31.0 | 35.1 | 42 | 14.7 | 18.9k | 156k |

The castle and battle runs use the `tools/playthrough.py` input list with `--fast-forward N --frames M --perf-log`.
The CSV's "game" column is frame minus render minus wait. It includes idle pacing, so it is not CPU cost.

What the numbers say:

- **The GPU is not the limit.** Village with MSAA 4x (24.7 ms) and render scale 150% (21.9 ms) match the plain runs.
  The menu renders in 6.8 ms at 640x360 and 7.3 ms at 2560x1440. The old 1440p crash did not reproduce.
- **Rendering on the CPU is the limit** in village and battle: about 0.7 µs per GX draw. Game logic plus command
  parsing was about 2.7 ms when last measured (the GX thread experiment, #212).
- **Draw calls are not the problem.** The port already merges draws: about 19k GX draws become about 300 D3D
  batches per frame. The cost is the work done for each GX draw and each vertex before batching.
- **Loading hitches:** single frames of 110-140 ms on level changes. In the Command Room run, one 138 ms frame was
  almost all outside render (level load) and a later 122 ms frame spent 112 ms inside it. The cause of the second
  is not measured; first-time shader compiles and texture decoding are the likely ones.
- **Possible regression:** #205 reported village at 16.7 ms with render about 8.2 ms on this PC. Today render is
  13-15 ms. This may be only the load. It must be checked on an idle PC before any optimisation.

## Likely costs

The village split below comes from earlier sessions' temporary timers (#185 to #213). It was not re-measured today,
because the built-in sampler is not usable for this (see task 1).

| Cost | Idea | Expected gain | Effort | Risk |
|---|---|---|---|---|
| Per-draw set-up in `DrawPrims` (shader key, constants, output state, record, vertex layout) x 19k | Fast path: if no BP, XF or CP register changed since the last draw, only decode vertices and append indices | 2-4 ms | medium | a missed state change draws wrongly |
| CPU attribute decode, about 35 ns/vertex, about 5 ms | SIMD or per-format decoders for the common layouts | 1-3 ms | medium | low |
| Same, for static meshes | Cache decoded geometry per display list, check the source arrays for changes | 3-6 ms | large | skinned and animated arrays |
| Queue vectors, then `memcpy` into mapped buffers (about 1 ms) | Write straight into the mapped ring segment | about 1 ms | medium | ring sync bugs (see #213) |
| Shader compile (`D3DCompile`) and texture decode on first use | Shader cache on disk, warm it at level load | may remove the render hitch | small | stale cache: key it by source hash |
| Frame-buffer copies | Already pooled (#177), small | none | - | - |
| Game thread and render run in series | Tried in #212, no gain: the game waits for draw-done. Do not repeat | none | - | - |

## Before numbers (task 1 done)

`--bench` runs at 1920x1080, headless on the GPU, `--no-ini --no-mods --no-audio`, empty save folder. Median of 3 runs; 1200 frames
each. Other sessions were still running playthroughs on the PC.

| Scene | avg | p95 | worst | render | decode | set-up | flush | copy |
|---|---|---|---|---|---|---|---|---|
| `--bench village` | 23.85 | 28.18 | 37.18 | 14.90 | 6.11 | 5.47 | 2.36 | 0.17 |
| `--bench battle` | 22.57 | 27.29 | 33.05 | 14.86 | 5.84 | 5.67 | 2.43 | 0.16 |
| `--bench castle` | 16.67 | 18.16 | 22.42 | 7.89 | 3.11 | 2.77 | 1.53 | 0.15 |

Noise: village avg 23.19-27.20 (one run had a 142 ms frame), battle 22.45-22.65, castle held 60 fps in all runs. The render split
shows decode and per-draw set-up about equal (each 5.5-6 ms in village and battle), flush about 2.4 ms.

## How to verify every task

1. Idle PC (no other builds or sessions). Three runs each of village and battle, before and after. Compare the median average, p95 and worst.
2. `--bench village --frames 4800 --screenshot` before and after must be pixel-identical (ignore the first run after a build).
3. `python tools/smoke.py`, `python tools/playthrough.py`, and `tools/mod_test.py` if textures change.

## Tasks, ranked (one PR each)

1. **Make measuring reliable.** Add `--bench battle` and `--bench castle`. Add decode, set-up, flush and copy columns to
   `--perf-log`. Then re-measure on an idle PC and, if village is still over 10 ms render, bisect from #205. Also fix:
   - `--bench village` only reaches the village at 1920x1080, because the pointer is given in window pixels. At other sizes it measures the menu.
   - `port_runtime.cpp` around line 1471: `PortSamplerStart()` was inserted between an `if` and `PortGfxStart()`.
     The condition now guards the sampler, and the renderer starts unconditionally.
   - The sampler samples all threads, so 98% of its samples are waits. Sample the game thread only.
2. **Level-change hitches.** Time shader compiles and texture decodes in the hitch frame, then add a shader cache on
   disk if compiles dominate. Gain: the 112 ms render frame goes away (estimate). Small. The 138 ms load frame is separate.
3. **Per-draw fast path** in `DrawPrims`. Gain: 2-4 ms in village and battle. Medium.
4. **Faster vertex decode and direct writes into the ring.** Gain: 2-3 ms. Medium.
5. **Display-list geometry cache**, only if village and battle still miss 60 fps after tasks 3 and 4. Large.

Tasks 3 and 4 together should bring render in village and battle to about 8-10 ms. That leaves headroom for 60 fps.
