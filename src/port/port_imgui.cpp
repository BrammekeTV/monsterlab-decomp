// PC platform layer: Dear ImGui (third_party/imgui, v1.92.9b, MIT: see its LICENSE.txt), built as one
// file with the platform layer: the core and the Win32 and Direct3D 11 backends. Used by the stats
// overlay and the settings screen (port_overlay.cpp).
#ifdef _WIN32
#include "../../third_party/imgui/imgui.cpp"
#include "../../third_party/imgui/imgui_draw.cpp"
#include "../../third_party/imgui/imgui_tables.cpp"
#include "../../third_party/imgui/imgui_widgets.cpp"
#include "../../third_party/imgui/backends/imgui_impl_dx11.cpp"
#include "../../third_party/imgui/backends/imgui_impl_win32.cpp"
#endif
