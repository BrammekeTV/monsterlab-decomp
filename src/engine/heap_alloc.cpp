// Engine heap: allocation.
// Original: fn_801EE7A4, fn_801EEB20 0x801EE7A4 - 0x801EF070 (auto_03_801E73E4_text.s)
//
// The engine's heaps (the main heap object at lbl_8047CAC0 and the second one at +0x394) are
// Doug Lea's malloc 2.7 with 16-byte alignment: this function is its _int_malloc, fn_801EF070
// is _int_free, fn_801EF1A8 is malloc_consolidate and fn_801EE7A4 is sYSMALLOc. The layout below
// is read off the asm.
//
// Heap state:  +0x000 max fast size; bit 0 = the heap has handed out chunks, bit 1 = the fast
//                     bins hold chunks
//              +0x004 fast bins (singly linked, by size / 8 - 2)
//              +0x030 top chunk, +0x034 last remainder
//              +0x038 bin 1 (unsorted); bin i is the list head at heap + 0x30 + i * 8
//              +0x338 bin map: one bit per bin that may hold chunks
//              +0x348 trim threshold, +0x34C top pad, +0x360 page size, +0x364 bit 0 = the
//                     memory it gets is contiguous, +0x368 mapped bytes, +0x36C bytes taken,
//                     +0x370 and +0x378 their peaks
//              +0x37C the heap's one memory block, +0x380 its size (HeapInit), +0x38C the
//                     end of that block once it has been handed to the allocator (0 before)
// Chunk:       +0x0 size of the previous chunk when that is free, +0x4 size | 1 when the
//              previous chunk is in use, +0x8 next and +0xC previous chunk while free.
//              The caller's memory starts at +0x8.
#include "types.h"

extern "C" {
void  fn_801EF1A8(u8* heap, u8* arg);               // 0x801EF1A8 (merge the fast bins / first-time init)
void  fn_801EF070(u8* heap, void* p);               // 0x801EF070 (free)
void* fn_801EEB20(u8* heap, u32 size);
}

#define CHUNK_SIZE(c) (*(u32*)((c) + 0x4))
#define CHUNK_FD(c)   (*(u8**)((c) + 0x8))
#define CHUNK_BK(c)   (*(u8**)((c) + 0xC))

// Bin number for a large chunk size (>= 0x100): 4 bins per power of two.
static inline u32 LargeBinIndex(u32 size) {
    u32 x = size >> 8;
    if (x >= 0x10000) return 0x5F;
    u32 n = ((x - 0x100) >> 16) & 8;
    x <<= n;
    u32 m = ((x - 0x1000) >> 16) & 4;
    n += m;
    x <<= m;
    m = ((x - 0x4000) >> 16) & 2;
    n += m;
    x <<= m;
    m = 13 - n + ((x >> 14) & ~(x >> 15));
    return 0x20 + (m << 2) + ((size >> (m + 6)) & 3);
}

// The allocator's "more core" source, inlined three times in fn_801EE7A4. A heap owns one fixed
// block. The first request for more than nothing gets the whole block (its 16-byte aligned start);
// after that a request for 0 bytes returns the block's end and anything else fails.
static inline u8* HeapMoreCore(u8* heap, s32 size) {
    if (*(u8**)(heap + 0x38C) == NULL) {
        u8* base = *(u8**)(heap + 0x37C);
        *(u8**)(heap + 0x38C) = base + *(s32*)(heap + 0x380);
        if (size > 0) {
            return (u8*)((((((uintptr_t)base + 0xFFF) & ~(uintptr_t)0xFFF) + 0x8) & ~(uintptr_t)0xF) - 0x8);
        }
    }
    if (size == 0) return *(u8**)(heap + 0x38C);
    return (u8*)(intptr_t)-1;
}

// 0x801EE7A4
// The top chunk cannot hold a chunk of nb bytes: merges the fast bins and retries if there are
// any; otherwise extends the top chunk with the heap's memory block (in practice: the first
// allocation takes the whole block) and allocates from it. Returns NULL when the block is used up.
extern "C" void* fn_801EE7A4(u8* heap, u32 nb, u8* av) {
    u8* const kFail = (u8*)(intptr_t)-1;
    u32 pageMask = *(u32*)(av + 0x360) - 1;
    if (*(u32*)(av + 0x0) & 2) {
        fn_801EF1A8(heap, av);
        return fn_801EEB20(heap, nb - 0xF);
    }

    u8* oldTop = *(u8**)(av + 0x30);
    u32 oldSize = CHUNK_SIZE(oldTop) & ~3u;
    u8* oldEnd = oldTop + oldSize;

    s32 size = (s32)(nb + *(u32*)(av + 0x34C) + 0x10);
    if (*(u32*)(av + 0x364) & 1) size -= (s32)oldSize;
    size = (s32)(((u32)size + pageMask) & ~pageMask);

    u8* brk = kFail;
    if (size > 0) brk = HeapMoreCore(heap, size);
    if (brk == kFail) return NULL;

    *(u32*)(av + 0x36C) += (u32)size;
    if (brk == oldEnd) {
        // The new memory continues the top chunk.
        CHUNK_SIZE(oldTop) = ((u32)size + oldSize) | 1;
    } else {
        u8* alignedBrk = brk;
        s32 correction = 0;
        u8* sndBrk;
        if ((*(u32*)(av + 0x364) & 1) && oldSize != 0 && brk < oldEnd) {
            *(u32*)(av + 0x364) &= ~1u;
        }
        if (*(u32*)(av + 0x364) & 1) {
            if (oldSize != 0) *(u32*)(av + 0x36C) += (u32)(brk - oldEnd);
            u32 misalign = ((u32)(uintptr_t)brk + 0x8) & 0xF;   // lint:ok (low bits only)
            if (misalign != 0) {
                correction = (s32)(0x10 - misalign);
                alignedBrk = brk + correction;
            }
            correction += (s32)oldSize;
            u8* brkEnd = brk + size;
            u32 end = (u32)(uintptr_t)brkEnd + (u32)correction;   // lint:ok (Wii address arithmetic, as the original)
            correction += (s32)(((end + pageMask) & ~pageMask) - end);
            sndBrk = HeapMoreCore(heap, correction);
            if (sndBrk == kFail) {
                correction = 0;
                sndBrk = HeapMoreCore(heap, 0);
                if (sndBrk != kFail) *(u32*)(av + 0x36C) += (u32)(sndBrk - brk) - (u32)size;
            } else if (sndBrk < brk) {
                *(u32*)(av + 0x364) &= ~1u;
                sndBrk = brkEnd;
                correction = 0;
            }
        } else {
            sndBrk = HeapMoreCore(heap, 0);
            *(u32*)(av + 0x36C) += (u32)(sndBrk - brk) - (u32)size;
        }

        if (sndBrk != kFail) {
            *(u8**)(av + 0x30) = alignedBrk;
            CHUNK_SIZE(alignedBrk) = ((u32)correction + (u32)(sndBrk - alignedBrk)) | 1;
            *(u32*)(av + 0x36C) += (u32)correction;
            if (oldSize != 0) {
                // The old top chunk is not next to the new memory: close it with two marker
                // words and free what is left of it.
                oldSize = (oldSize - 0xC) & ~0xFu;
                CHUNK_SIZE(oldTop) = oldSize | 1;
                *(u32*)(oldTop + oldSize + 0x4) = 5;
                *(u32*)(oldTop + oldSize + 0x8) = 5;
                if (oldSize >= 0x10) {
                    u32 saved = *(u32*)(av + 0x348);
                    *(u32*)(av + 0x348) = 0xFFFFFFFFu;
                    fn_801EF070(heap, oldTop + 0x8);
                    *(u32*)(av + 0x348) = saved;
                }
            }
        }
    }

    u32 taken = *(u32*)(av + 0x36C);
    if (taken > *(u32*)(av + 0x370)) *(u32*)(av + 0x370) = taken;
    taken += *(u32*)(av + 0x368);
    if (taken > *(u32*)(av + 0x378)) *(u32*)(av + 0x378) = taken;

    u8* p = *(u8**)(av + 0x30);
    u32 csize = CHUNK_SIZE(p) & ~3u;
    if (csize >= nb + 0x10) {
        u8* rem = p + nb;
        *(u8**)(av + 0x30) = rem;
        CHUNK_SIZE(p) = nb | 1;
        CHUNK_SIZE(rem) = (csize - nb) | 1;
        return p + 0x8;
    }
    return NULL;
}

// 0x801EEB20
// Allocates size bytes from the heap; returns NULL when the request is too large or the heap
// cannot grow.
extern "C" void* fn_801EEB20(u8* heap, u32 size) {
    if (size >= (u32)-0x21) return NULL;
    u32 nb = size + 0x13 < 0x10 ? 0x10 : (size + 0x13) & ~0xFu;      // chunk size: header + 16-byte alignment

    u32 maxFast = *(u32*)(heap + 0x0);
    u32 idx = 0;
    u8* unsorted = heap + 0x38;
    if (!(maxFast & 1)) {
        // Nothing was ever allocated: set the heap up on first use, then carve from the top chunk.
        if (maxFast == 0) fn_801EF1A8(heap, heap);
        goto use_top;
    }

    if (nb <= maxFast) {
        // Small request: reuse a chunk of exactly this size from its fast bin.
        u8** bin = (u8**)(heap + 0x4 + ((nb >> 3) - 2) * 4);
        u8* victim = *bin;
        if (victim != NULL) {
            *bin = CHUNK_FD(victim);
            return victim + 0x8;
        }
    }

    if (nb < 0x100) {
        // Small bins hold chunks of one size each.
        idx = nb >> 3;
        u8* bin = heap + 0x30 + (nb & ~7u);
        u8* victim = CHUNK_BK(bin);
        if (victim != bin) {
            u8* bck = CHUNK_BK(victim);
            CHUNK_SIZE(victim + nb) |= 1;
            CHUNK_BK(bin) = bck;
            CHUNK_FD(bck) = bin;
            return victim + 0x8;
        }
    } else {
        idx = LargeBinIndex(nb);
        if (maxFast & 2) fn_801EF1A8(heap, heap);       // merge the fast bins before a large request
    }

    // Go through the recently freed chunks: take an exact fit, split the last remainder for a
    // small request, and sort everything else into its bin.
    for (u8* victim; (victim = CHUNK_BK(unsorted)) != unsorted;) {
        u32 csize = CHUNK_SIZE(victim) & ~3u;
        u8* bck = CHUNK_BK(victim);

        if (nb < 0x100 && bck == unsorted && victim == *(u8**)(heap + 0x34) && csize > nb + 0x10) {
            u8* rem = victim + nb;
            u32 remSize = csize - nb;
            CHUNK_FD(unsorted) = rem;
            CHUNK_BK(unsorted) = rem;
            *(u8**)(heap + 0x34) = rem;
            CHUNK_FD(rem) = unsorted;
            CHUNK_BK(rem) = unsorted;
            CHUNK_SIZE(victim) = nb | 1;
            CHUNK_SIZE(rem) = remSize | 1;
            *(u32*)(rem + remSize) = remSize;
            return victim + 0x8;
        }

        CHUNK_BK(unsorted) = bck;
        CHUNK_FD(bck) = unsorted;
        if (csize == nb) {
            CHUNK_SIZE(victim + csize) |= 1;
            return victim + 0x8;
        }

        u32 vidx;
        u8* fwd;
        if (csize < 0x100) {
            vidx = csize >> 3;
            bck = heap + 0x30 + (csize & ~7u);
            fwd = CHUNK_FD(bck);
        } else {
            vidx = LargeBinIndex(csize);
            bck = heap + 0x30 + vidx * 8;
            fwd = CHUNK_FD(bck);
            if (fwd != bck) {
                // Large bins are kept sorted, largest first.
                if (csize < CHUNK_SIZE(CHUNK_BK(bck))) {
                    fwd = bck;
                    bck = CHUNK_BK(bck);
                } else if (csize >= 0x100) {
                    u32 key = csize | 1;
                    while (key < CHUNK_SIZE(fwd)) fwd = CHUNK_FD(fwd);
                    bck = CHUNK_BK(fwd);
                }
            }
        }
        *(u32*)(heap + 0x338 + ((s32)vidx >> 5) * 4) |= 1u << (vidx & 31);
        CHUNK_BK(victim) = bck;
        CHUNK_FD(victim) = fwd;
        CHUNK_BK(fwd) = victim;
        CHUNK_FD(bck) = victim;
    }

    if (nb >= 0x100) {
        // Large request: the smallest chunk in its own bin that fits.
        u8* bin = heap + 0x30 + idx * 8;
        for (u8* victim = CHUNK_BK(bin); victim != bin; victim = CHUNK_BK(victim)) {
            u32 csize = CHUNK_SIZE(victim) & ~3u;
            if (csize < nb) continue;
            u32 remSize = csize - nb;
            u8* fd = CHUNK_FD(victim);
            u8* bk = CHUNK_BK(victim);
            CHUNK_BK(fd) = bk;
            CHUNK_FD(bk) = fd;
            if (remSize < 0x10) {
                CHUNK_SIZE(victim + csize) |= 1;
                return victim + 0x8;
            }
            u8* rem = victim + nb;
            CHUNK_FD(unsorted) = rem;
            CHUNK_BK(unsorted) = rem;
            CHUNK_FD(rem) = unsorted;
            CHUNK_BK(rem) = unsorted;
            CHUNK_SIZE(victim) = nb | 1;
            CHUNK_SIZE(rem) = remSize | 1;
            *(u32*)(rem + remSize) = remSize;
            return victim + 0x8;
        }
    }

    {
        // Otherwise the smallest chunk of any larger bin, found through the bin map.
        idx++;
        u8* bin = heap + 0x30 + idx * 8;
        u32 block = idx >> 5;
        u32 map = *(u32*)(heap + 0x338 + block * 4);
        u32 bit = 1u << (idx & 31);
        for (;;) {
            if (bit > map || bit == 0) {
                do {
                    if (++block >= 3) goto use_top;
                    map = *(u32*)(heap + 0x338 + block * 4);
                } while (map == 0);
                bin = heap + 0x30 + (block << 5) * 8;
                bit = 1;
            }
            while ((bit & map) == 0) {
                bit <<= 1;
                bin += 8;
            }
            u8* victim = CHUNK_BK(bin);
            if (victim == bin) {
                // The bin is empty after all: clear its bit and look further.
                map &= ~bit;
                *(u32*)(heap + 0x338 + block * 4) = map;
                bit <<= 1;
                bin += 8;
                continue;
            }
            u32 csize = CHUNK_SIZE(victim) & ~3u;
            u8* bck = CHUNK_BK(victim);
            CHUNK_BK(bin) = bck;
            CHUNK_FD(bck) = bin;
            u32 remSize = csize - nb;
            if (remSize < 0x10) {
                CHUNK_SIZE(victim + csize) |= 1;
                return victim + 0x8;
            }
            u8* rem = victim + nb;
            CHUNK_FD(unsorted) = rem;
            CHUNK_BK(unsorted) = rem;
            CHUNK_FD(rem) = unsorted;
            CHUNK_BK(rem) = unsorted;
            if (nb < 0x100) *(u8**)(heap + 0x34) = rem;
            CHUNK_SIZE(victim) = nb | 1;
            CHUNK_SIZE(rem) = remSize | 1;
            *(u32*)(rem + remSize) = remSize;
            return victim + 0x8;
        }
    }

use_top:
    {
        // Carve the request off the top chunk, the free space at the end of the heap.
        u8* victim = *(u8**)(heap + 0x30);
        u32 csize = CHUNK_SIZE(victim) & ~3u;
        if (csize >= nb + 0x10) {
            u8* rem = victim + nb;
            *(u8**)(heap + 0x30) = rem;
            CHUNK_SIZE(victim) = nb | 1;
            CHUNK_SIZE(rem) = (csize - nb) | 1;
            return victim + 0x8;
        }
        return fn_801EE7A4(heap, nb, heap);
    }
}
