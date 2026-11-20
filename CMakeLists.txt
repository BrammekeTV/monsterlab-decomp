cmake_minimum_required(VERSION 3.20)
project(MonsterLabPC LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Decompiled game code. For now this is compiled only (an OBJECT library):
# the platform layer that replaces the Wii SDK doesn't exist yet, so nothing links.
file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.c)
add_library(game OBJECT ${GAME_SOURCES})
target_include_directories(game PUBLIC include)

if(MSVC)
  target_compile_options(game PRIVATE /W3 /permissive- /utf-8
    /wd4244 /wd4267 /wd4305)   # narrowing warnings from the original 32-bit code
  target_compile_definitions(game PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(game PRIVATE -Wall -Wno-unused-variable -Wno-unused-function)
endif()

# Link check: links every object into an exe so the linker lists what is still missing
# (unresolved functions/globals, duplicate definitions, linkage mismatches). The exe is not
# runnable. CI feeds the linker output to tools/missing_symbols.py; the link may fail.
add_executable(monsterlab_link $<TARGET_OBJECTS:game>)
set_target_properties(monsterlab_link PROPERTIES LINKER_LANGUAGE CXX)
if(MSVC)
  target_link_options(monsterlab_link PRIVATE /NOLOGO)
endif()

# PC port loader test (tests/sir_loader_test.cpp): `--selftest` needs no game data and runs in CI;
# `--dump <global_binary.sir>` is compared with the Python reference by tools/loader_check.py.
add_executable(sir_loader_test tests/sir_loader_test.cpp src/port/sir_loader.cpp)
target_include_directories(sir_loader_test PRIVATE include)
if(MSVC)
  target_compile_options(sir_loader_test PRIVATE /W3 /permissive- /utf-8)
  target_compile_definitions(sir_loader_test PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
