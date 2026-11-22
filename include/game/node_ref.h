#pragma once
#include "types.h"

// Node references: one 32-bit word of the action program that the console code reads in several
// ways (kind byte at +0, byte at +1, halfword at +2, or the whole word as an entity id). The word
// is read as a whole and the fields are taken from it, so the code does not depend on byte order.
//   byte 0 (bits 24-31): kind in the low nibble, high nibble = sub-field, bit 7 = "whole word is an id"
//   byte 1 (bits 16-23), halfword at +2 (bits 0-15)

static inline u32 NodeRefWord(const u8* ref) { return *(const u32*)ref; }
static inline u8  NodeRefByte0(u32 w) { return (u8)(w >> 24); }
static inline u8  NodeRefByte1(u32 w) { return (u8)(w >> 16); }
static inline u16 NodeRefHalf2(u32 w) { return (u16)(w & 0xFFFF); }
static inline s32 NodeRefIsId(u32 w) { return (s32)((w >> 31) & 1); }
// Kind: 1 when bit 7 of byte 0 is set, else its low nibble.
static inline u32 NodeRefKind(u32 w) { return NodeRefIsId(w) ? 1u : ((w >> 24) & 0xFu); }
// High nibble of byte 0, 0 when bit 7 is set.
static inline u32 NodeRefNibble(u32 w) { return NodeRefIsId(w) ? 0u : ((w >> 28) & 0xFu); }
// Stores byte 0 of a reference, leaving the other three bytes alone.
static inline void NodeRefSetByte0(u8* ref, u8 b) {
    *(u32*)ref = (*(u32*)ref & 0x00FFFFFFu) | ((u32)b << 24);
}
