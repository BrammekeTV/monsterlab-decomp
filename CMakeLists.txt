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
