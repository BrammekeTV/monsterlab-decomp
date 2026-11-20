cmake_minimum_required(VERSION 3.20)
project(MonsterLabPC LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Decompiled game code and the PC platform layer (src/port/), as an OBJECT library.
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
# (unresolved functions/globals, duplicate definitions, linkage mismatches). This link fails by
# design: it has no port_gen.obj. CI feeds the linker output to tools/missing_symbols.py.
add_executable(monsterlab_link $<TARGET_OBJECTS:game>)
set_target_properties(monsterlab_link PROPERTIES LINKER_LANGUAGE CXX)
if(MSVC)
  target_link_options(monsterlab_link PRIVATE /NOLOGO)
endif()

# The PC executable (docs/port_runtime.md): the same objects plus port_gen.obj, which
# tools/port_link.py generates from the objects' symbol tables. It resolves every global the
# sources do not define to its original Wii address and every function that is not decompiled yet
# to a stub that reports its name and stops. The runtime maps the Wii address space at
# 0x80000000 / 0x90000000, so the exe needs the upper 2 GB and a fixed base.
#   monsterlab.exe --selftest     no game data needed (CI)
#   monsterlab.exe                runs main() with the user's own disc files in orig/RM7P4F
find_package(Python3 COMPONENTS Interpreter)
if(MSVC AND Python3_FOUND)
  set(PORT_GEN_DIR ${CMAKE_BINARY_DIR}/port_gen)
  file(GENERATE OUTPUT ${PORT_GEN_DIR}/objects_$<CONFIG>.rsp CONTENT "$<JOIN:$<TARGET_OBJECTS:game>,
>
")
  add_custom_command(
    OUTPUT ${PORT_GEN_DIR}/port_gen.obj
    COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tools/port_link.py
            --objs ${PORT_GEN_DIR}/objects_$<CONFIG>.rsp --out ${PORT_GEN_DIR}
    DEPENDS $<TARGET_OBJECTS:game> ${CMAKE_SOURCE_DIR}/tools/port_link.py
            ${CMAKE_SOURCE_DIR}/config/RM7P4F/symbols.txt
    COMMENT "Generating port_gen.obj (globals, stubs, function table)"
    VERBATIM)
  set_source_files_properties(${PORT_GEN_DIR}/port_gen.obj PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
  add_executable(monsterlab $<TARGET_OBJECTS:game> ${PORT_GEN_DIR}/port_gen.obj)
  set_target_properties(monsterlab PROPERTIES LINKER_LANGUAGE CXX)
  target_link_options(monsterlab PRIVATE /NOLOGO /FIXED /BASE:0x400000 /DYNAMICBASE:NO
    /LARGEADDRESSAWARE /STACK:0x400000 /INCREMENTAL:NO /MAP)
endif()

# PC port loader test (tests/sir_loader_test.cpp): `--selftest` needs no game data and runs in CI;
# `--dump <global_binary.sir>` is compared with the Python reference by tools/loader_check.py.
add_executable(sir_loader_test tests/sir_loader_test.cpp src/port/sir_loader.cpp)
target_include_directories(sir_loader_test PRIVATE include)
if(MSVC)
  target_compile_options(sir_loader_test PRIVATE /W3 /permissive- /utf-8)
  target_compile_definitions(sir_loader_test PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
