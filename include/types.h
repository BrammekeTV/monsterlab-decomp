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

// PC port savestates (docs/savestates.md): the statics of every source that does not include port/port.h
// (the decompiled game code) go into two sections of their own, which a state saves and restores whole.
// Globals moved to their original address (ORIG) live in the mapped Wii memory and are saved with it.
#if defined(_MSC_VER) && defined(_M_IX86)
#pragma bss_seg(".savb")
#pragma data_seg(".savd")
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

// PowerPC slw/srw: the shift amount is its low 6 bits; 32..63 gives 0 (not undefined like C).
static inline u32 PpcSlw(u32 v, u32 n) { return (n & 0x20) ? 0u : v << (n & 0x1F); }
static inline u32 PpcSrw(u32 v, u32 n) { return (n & 0x20) ? 0u : v >> (n & 0x1F); }
static inline s32 PpcSraw(s32 v, u32 n) { return (n & 0x20) ? (v < 0 ? -1 : 0) : v >> (n & 0x1F); }

// Marks the original address of a decompiled function / variable.
#define ORIG(addr)

// The body of a loop that waits for an interrupt handler to change memory. On the console it is
// empty: the interrupt arrives between two instructions. The PC build has no preemption, so the
// loop has to let the interrupt thread run (PortInterruptPoint, docs/port_runtime.md).
extern "C" void PortInterruptPoint(void);
#define WAIT_FOR_INTERRUPT() PortInterruptPoint()
