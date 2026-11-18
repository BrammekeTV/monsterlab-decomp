#pragma once
// Fixed-width types matching the Wii SDK's naming.
#include <stddef.h>
#include <stdint.h>

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;
typedef int      BOOL;

// The decompiled code reads game structs at their original offsets with 4-byte pointer slots, so the
// PC build must be 32-bit (Win32) for now. x64 comes later, after named structs and a data loader
// replace the raw offsets (README, "Roadmap", steps 1 and 6).
static_assert(sizeof(void*) == 4, "build for 32-bit (cmake -A Win32); see README roadmap");
// Return value of script callbacks whose signature type is 0x03/0x2E: an int, bool or a pointer
// (string, texture, buffer). Pointer-sized so a 64-bit build does not truncate it.
typedef intptr_t ScriptValue;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

// Marks the original address of a decompiled function / variable.
#define ORIG(addr)
