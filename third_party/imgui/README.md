# Dear ImGui

Version **1.92.9b**, from the release tag `v1.92.9b` of <https://github.com/ocornut/imgui>
(commit `f1cc2ae15e53a861a874c3034aae6798fde194ab`), unmodified. MIT licence: `LICENSE.txt`.

Kept: the core (`imgui*.cpp/.h`, `imstb_*.h`, `imconfig.h`) and the Win32 and Direct3D 11
backends (`backends/`). Left out: the demo, examples, docs, misc and the other backends.

The PC port builds it as one file, `src/port/port_imgui.cpp`, so `tools/build_pc.py` (with `/O2`)
and CMake pick it up with the rest of `src/port/`.
