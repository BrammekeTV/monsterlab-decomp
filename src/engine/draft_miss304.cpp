// Drafts miss304: lightning path displacement, hook argument resolver, settings pool setup,
// particle/line draw passes and a camera-list pass.
#include <stdint.h>

#include "types.h"
#include "revolution.h"

extern "C++" u8* gGlobalSettings;     // 0x805FB7A4 (pointer to the settings block)

extern "C" {
u32   RandomU32(void);                                       // 0x80143460
void* MemAllocDefault(u32 size);                             // 0x801D4650
s32   HeapInit(u8* arg0, u8* arg1, u32 arg2);                // 0x801EF8DC
void  fn_800446BC(void* cam, s32 v);                         // 0x800446BC
void  fn_8010AB0C(s32 a, s32 b);                             // 0x8010AB0C
void  fn_8014E3EC(s32 n);                                    // 0x8014E3EC
s32   fn_80152620(s32 a, s32 b, u8* quad);                   // 0x80152620
void  fn_80155724(u8* m);                                    // 0x80155724
void* fn_801B7198(void* anim, s32 a);                        // 0x801B7198
u32   fn_801CB96C(u8* list);                                 // 0x801CB96C (leaf)
u32   fn_801CBCDC(u8* list, u8* e);                          // 0x801CBCDC (leaf)
void  fn_801D82AC(u8* arg0, u8* node, u8* vec, u32 c, u32 d, f32 f1, f32 f2); // 0x801D82AC
u8*   fn_801D5850(void);                                     // 0x801D5850
void  fn_801E2BB4(u8* e);                                    // 0x801E2BB4
u32   fn_801E29C8(u8* p);                                    // 0x801E29C8
void  fn_801E32E4(u8* e);                                    // 0x801E32E4
void  fn_801EBA20(void* list, void* item);                   // 0x801EBA20
s32   fn_801ED368(u8* ref);                                  // 0x801ED368
void  fn_801EE078(u8* params, void* a, void* b);             // 0x801EE078

extern u8  lbl_8047C640[];     // 0x8047C640
extern u8  lbl_8047CAC0[];     // 0x8047CAC0 (main heap; second heap at +0x394)
extern u8  lbl_805F9212;       // 0x805F9212 (copy filter value)
extern s32 lbl_805FB4C4;       // 0x805FB4C4 (vertex buffer write offset)
extern s32 lbl_805FB4CC;       // 0x805FB4CC (vertex buffer bytes left)
extern u8* lbl_805FB4D0;       // 0x805FB4D0 (vertex buffer base)
extern u8  lbl_805FB50C;       // 0x805FB50C (GX shadow flags byte)
extern void* lbl_805FB5F8;     // 0x805FB5F8 (current camera)
extern s32* lbl_805FB798;      // 0x805FB798 (per-pool requested byte counters)
extern s32* lbl_805FB79C;      // 0x805FB79C (per-pool allocation counters)
extern s32* lbl_805FB7A0;      // 0x805FB7A0
extern u8  lbl_805FB7A8[];     // 0x805FB7A8 (+0 MEM2 heap base, +4 second heap enabled)
extern u8* lbl_805FB814;       // 0x805FB814 (list of queued line records)
extern u8* lbl_805FB818;       // 0x805FB818 (list of queued quad records)
extern u8* lbl_805FB830;       // 0x805FB830 (list of queued beam records)
extern u8  lbl_805FB89C[];     // 0x805FB89C (script result register, 32 bits)
}

static const u32 kHashThis   = 0xC87B6F93u;  // "this"
static const u32 kHashOther  = 0x2E0CF790u;  // "Other"
static const u32 kHashResult = 0xE3920695u;  // "Result"
static const u32 kHashNull   = 0x135E11FBu;  // "NULL"

// Clamp v into the interval given by (lo, hi); a reversed interval snaps to the nearer end.
static f32 ClampPair(f32 v, f32 lo, f32 hi) {
    if (lo < hi) {
        if (v < lo) return lo;
        if (v > hi) v = hi;
        return v;
    }
    if (v < hi) return hi;
    if (v > lo) return lo;
    return v;
}

// Original stores 32-bit script slots; the PC port keeps them 32 bits wide.
static u32 PtrSlot(const void* p) {
    return (u32)(uintptr_t)p;  // lint:ok
}

// Writes one 0x61 BP register word to the GX FIFO.
static void BP(u32 v) {
    GXWGFifo._u8  = 0x61;
    GXWGFifo._u32 = v;
}

// Re-issues the cached BP state when GX flag bit 1 is set: bit 1 is cleared and one of two
// register sequences is written depending on flag bit 0.
static void GxRestoreDrawState(void) {
    u8 flags = lbl_805FB50C;
    if (flags & 2) {
        lbl_805FB50C = (u8)(flags & ~2);
        if (flags & 1) {
            BP(0xFE00000Fu);
            BP(0);
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x103F; GXWGFifo._u32 = 0;
            BP(0xFE003C00u);
            BP(0);
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x1010; GXWGFifo._u32 = 0x441;
            BP(0x28380000u);
            BP(0xC008F2A4u);
            BP(0xC108E6F0u);
        } else {
            BP(0xFE00000Fu);
            BP(0);
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x103F; GXWGFifo._u32 = 0;
            BP(0xFE003C00u);
            BP(0);
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x100E; GXWGFifo._u32 = 0x442;
            GXWGFifo._u8 = 0x10; GXWGFifo._u16 = 0; GXWGFifo._u16 = 0x1010; GXWGFifo._u32 = 0x442;
            BP(0x28380000u);
            BP(0xC008FFFAu);
            BP(0xC108FFD0u);
        }
    }
}

// Copies the 4 bytes at src to dst in reverse byte order (color word swap).
static void CopyColorSwapped(u8* dst, const u8* src) {
    u8 b0 = src[0], b1 = src[1], b2 = src[2], b3 = src[3];
    dst[0] = b3;
    dst[1] = b2;
    dst[2] = b1;
    dst[3] = b0;
}

// Reserves `size` bytes in the vertex buffer (making room first if needed) and returns the slot.
static u8* ReserveVertexBytes(s32 size) {
    if (lbl_805FB4CC < size) fn_8014E3EC(size);
    s32 off = lbl_805FB4C4;
    u8* base = lbl_805FB4D0;
    lbl_805FB4C4 = off + size;
    lbl_805FB4CC -= size;
    return base + off;
}

extern "C" {

// 0x801BC970
// Recursive midpoint displacement of the lightning path (points stride 0xC from +0x4C): the middle
// point between a1 and a2 is offset randomly (amplitude shrinks by half per level), clamped to the
// bolt radius, then recursed on both halves. farg0 is the current amplitude scale.
void fn_801BC970(u8* arg0, s32 arg1, s32 arg2, f32 farg0) {
    s32 n = arg2 - arg1;
    s32 mid = arg1 + ((s32)((u32)n >> 31) + n >> 1);
    if (n > 1) {
        f32 scale = farg0 * *(f32*)(arg0 + 0x34);
        u8* p1 = arg0 + arg1 * 0xC;
        u8* p2 = arg0 + arg2 * 0xC;
        f32 amp = scale * *(f32*)(*(u8**)arg0 + 0x4C);
        f32 hx = (*(f32*)(p1 + 0x4C) + *(f32*)(p2 + 0x4C)) * 0.5f;
        f32 hy = (*(f32*)(p1 + 0x50) + *(f32*)(p2 + 0x50)) * 0.5f;
        f32 hz = (*(f32*)(p1 + 0x54) + *(f32*)(p2 + 0x54)) * 0.5f;
        f32 rx = amp * (2.0f * (2.3283064e-10f * (f32)RandomU32())) - amp;
        f32 ry = amp * (2.0f * (2.3283064e-10f * (f32)RandomU32())) - amp;
        f32 rz = amp * (2.0f * (2.3283064e-10f * (f32)RandomU32())) - amp;
        u8* pm = arg0 + mid * 0xC;
        f32 x = *(f32*)(pm + 0x4C) + rx;
        f32 y = *(f32*)(pm + 0x50) + ry;
        f32 z = *(f32*)(pm + 0x54) + rz;
        x -= hx;
        y -= hy;
        z -= hz;
        f32 lo = -scale;
        x = ClampPair(x, lo, scale);
        y = ClampPair(y, lo, scale);
        z = ClampPair(z, lo, scale);
        *(f32*)(pm + 0x4C) = x + hx;
        *(f32*)(pm + 0x50) = y + hy;
        *(f32*)(pm + 0x54) = z + hz;
        if (*(s32*)(*(u8**)arg0 + 0x58) != 0) {
            u32 cnt = *(u32*)(arg0 + 0x38);
            u8* last = arg0 + (cnt - 1) * 0xC;
            f32 z0 = *(f32*)(arg0 + 0x54);
            f32 dz = *(f32*)(last + 0x54) - z0;
            f32 target = z0 + ((f32)mid / (f32)cnt) * dz;
            if (*(f32*)(pm + 0x54) < target) {
                *(f32*)(pm + 0x54) = target;
            }
        }
        if (n > 2) {
            fn_801BC970(arg0, arg1, mid, 0.5f * farg0);
            fn_801BC970(arg0, mid, arg2, 0.5f * farg0);
        }
    }
}

// 0x801D2C00
// Stores the settings block, allocates the three per-pool counter tables (sum of the pool counts
// + 1 entries each) and sets up the second heap from the end of MEM2 up to the settings-derived
// start address.
void fn_801D2C00(u8* arg0) {
    gGlobalSettings = arg0;
    u8* s = arg0;
    s32 pools = *(s32*)(s + 0x74) + *(s32*)(s + 0x70) + *(s32*)(s + 0x98) + *(s32*)(s + 0x8C) +
                *(s32*)(s + 0x7C) + *(s32*)(s + 0x94) + *(s32*)(s + 0x90) + *(s32*)(s + 0x88) +
                *(s32*)(s + 0x84) + *(s32*)(s + 0x68) + *(s32*)(s + 0x80) + *(s32*)(s + 0x78) +
                *(s32*)(s + 0x6C);
    lbl_805FB79C = (s32*)MemAllocDefault((u32)((pools + 1) * 4));
    s = gGlobalSettings;
    pools = *(s32*)(s + 0x74) + *(s32*)(s + 0x70) + *(s32*)(s + 0x98) + *(s32*)(s + 0x8C) +
            *(s32*)(s + 0x7C) + *(s32*)(s + 0x94) + *(s32*)(s + 0x90) + *(s32*)(s + 0x88) +
            *(s32*)(s + 0x84) + *(s32*)(s + 0x68) + *(s32*)(s + 0x80) + *(s32*)(s + 0x78) +
            *(s32*)(s + 0x6C);
    lbl_805FB798 = (s32*)MemAllocDefault((u32)((pools + 1) * 4));
    s = gGlobalSettings;
    pools = *(s32*)(s + 0x74) + *(s32*)(s + 0x70) + *(s32*)(s + 0x98) + *(s32*)(s + 0x8C) +
            *(s32*)(s + 0x7C) + *(s32*)(s + 0x94) + *(s32*)(s + 0x90) + *(s32*)(s + 0x88) +
            *(s32*)(s + 0x84) + *(s32*)(s + 0x68) + *(s32*)(s + 0x80) + *(s32*)(s + 0x78) +
            *(s32*)(s + 0x6C);
    lbl_805FB7A0 = (s32*)MemAllocDefault((u32)((pools + 1) * 4));

    s = gGlobalSettings;
    u8* end = *(u8**)lbl_805FB7A8 + 0x3380000;
    u32 total = (u32)(*(s32*)(s + 0x0) + *(s32*)(s + 0x4) + *(s32*)(s + 0x34) * *(s32*)(s + 0x8) +
                      (*(s32*)(s + 0x38) - 1) * *(s32*)(s + 0xC) +
                      (*(s32*)(s + 0x3C) - 1) * *(s32*)(s + 0x10) +
                      (*(s32*)(s + 0x40) - 1) * *(s32*)(s + 0x14) +
                      (*(s32*)(s + 0x44) - 1) * *(s32*)(s + 0x18) +
                      (*(s32*)(s + 0x48) - 1) * *(s32*)(s + 0x1C) +
                      *(s32*)(s + 0x4C) * *(s32*)(s + 0x20) + *(s32*)(s + 0x50) * *(s32*)(s + 0x24) +
                      *(s32*)(s + 0x58) * *(s32*)(s + 0x28) + *(s32*)(s + 0x64) * *(s32*)(s + 0x2C));
    u8* start = (u8*)0x90400000 + total;
    if (end >= start) {
        if (end == start) {
            *(s32*)(lbl_805FB7A8 + 4) = 0;
            return;
        }
        // The original stores the start address; readers only test the slot for non-zero.
        *(s32*)(lbl_805FB7A8 + 4) = 1;
        HeapInit(lbl_8047CAC0 + 0x394, start, (u32)(end - start));
    }
}

// 0x801D6FD0
// Flushes the queued line records: for each record in the list at lbl_805FB814 (when a camera
// exists) packs a 0x20-byte vertex (swapped color + two 3-float endpoints) into the vertex buffer,
// restores the draw state, submits it and releases the record.
void fn_801D6FD0(void) {
    if (lbl_805FB814 != NULL) {
        fn_80155724(fn_801D5850());
        u8* link = *(u8**)(lbl_805FB814 + 0x14);
        u8* node = link != NULL ? link + 8 : NULL;
        while (node != NULL) {
            u8* nextLink = *(u8**)(node - 4);
            u8* next = nextLink != NULL ? nextLink + 8 : NULL;
            if (lbl_805FB5F8 != NULL) {
                u8* dst = ReserveVertexBytes(0x20);
                CopyColorSwapped(dst + 0x0, node + 0x18);
                *(f32*)(dst + 0x4)  = *(f32*)(node + 0x0);
                *(f32*)(dst + 0x8)  = *(f32*)(node + 0x4);
                *(f32*)(dst + 0xC)  = *(f32*)(node + 0x8);
                CopyColorSwapped(dst + 0x10, node + 0x18);
                *(f32*)(dst + 0x14) = *(f32*)(node + 0xC);
                *(f32*)(dst + 0x18) = *(f32*)(node + 0x10);
                *(f32*)(dst + 0x1C) = *(f32*)(node + 0x14);
                GxRestoreDrawState();
                fn_80152620(0xA8, 2, dst);
            }
            fn_801EBA20(lbl_805FB814, node);
            node = next;
        }
    }
}

// 0x801D7350
// Flushes the queued quad records at lbl_805FB818 (needs a camera): clears GX flag bit 3, sets an
// identity 3x4 matrix, packs each record into a 0x40-byte vertex (three positions with swapped
// colors) and submits it, then sets flag bit 3 again and restores the BP copy-filter register.
void fn_801D7350(void) {
    if (lbl_805FB5F8 == NULL || lbl_805FB818 == NULL) return;

    lbl_805FB50C = (u8)(lbl_805FB50C & ~8);
    BP(0xFE00C000u);
    BP(0);
    GxRestoreDrawState();

    f32 m[12] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    fn_80155724((u8*)m);

    u8* link = *(u8**)(lbl_805FB818 + 0x14);
    u8* node = link != NULL ? link + 8 : NULL;
    while (node != NULL) {
        u8* nextLink = *(u8**)(node - 4);
        u8* next = nextLink != NULL ? nextLink + 8 : NULL;
        u8* dst = ReserveVertexBytes(0x40);
        *(f32*)(dst + 0x4)  = *(f32*)(node + 0x0);
        *(f32*)(dst + 0x8)  = *(f32*)(node + 0x4);
        *(f32*)(dst + 0xC)  = *(f32*)(node + 0x8);
        CopyColorSwapped(dst + 0x0, node + 0x24);
        *(f32*)(dst + 0x14) = *(f32*)(node + 0xC);
        *(f32*)(dst + 0x18) = *(f32*)(node + 0x10);
        *(f32*)(dst + 0x1C) = *(f32*)(node + 0x14);
        CopyColorSwapped(dst + 0x10, node + 0x24);
        *(f32*)(dst + 0x24) = *(f32*)(node + 0x18);
        *(f32*)(dst + 0x28) = *(f32*)(node + 0x1C);
        *(f32*)(dst + 0x2C) = *(f32*)(node + 0x20);
        CopyColorSwapped(dst + 0x20, node + 0x24);
        fn_80152620(0x90, 3, dst);
        fn_801EBA20(lbl_805FB818, node);
        node = next;
    }

    lbl_805FB50C |= 8;
    BP(0xFE00C000u);
    BP((u32)lbl_805F9212 << 14);
}

// 0x801D88B0
// Draws and releases each queued beam record in the list at lbl_805FB830 via fn_801D82AC.
void fn_801D88B0(u8* arg0) {
    if (lbl_805FB830 != NULL) {
        u8* link = *(u8**)(lbl_805FB830 + 0x14);
        u8* node = link != NULL ? link + 8 : NULL;
        while (node != NULL) {
            u8* nextLink = *(u8**)(node - 4);
            u8* next = nextLink != NULL ? nextLink + 8 : NULL;
            fn_801D82AC(arg0, node, node + 0xC, *(u32*)(node + 0x20), *(u8*)(node + 0x24),
                        *(f32*)(node + 0x18), *(f32*)(node + 0x1C));
            fn_801EBA20(lbl_805FB830, node);
            node = next;
        }
    }
}

// 0x801E2470
// Resolves hook argument slots in place. Mask bits 0-7: object hash -> pointer ("this", "Other",
// "Result", "NULL" or a looked-up object; slot 0 must not resolve to null). Bits 8-15: resolve a
// reference through fn_801EE078/fn_801ED368. Bits 16-23: take the first value stored behind the
// object's +0xA4 pointer. Bits 24-31: replace a "Result" hash with the result register.
// Returns 0 if a required slot failed to resolve.
s32 fn_801E2470(u32* arg0, u32 arg1, u32 arg2, void* arg3, void* arg4) {
    u32* slots = arg0;
    s32 ok = 1;

    if ((u8)arg2 != 0) {
        for (u32 i = 0; i < arg1; i++) {
            if (arg2 & (1u << i)) {
                u32 h = slots[i];
                s32 isNull = 0;
                if (h == kHashThis) {
                    slots[i] = PtrSlot(arg3);
                } else if (h == kHashOther) {
                    slots[i] = PtrSlot(arg4);
                } else if (h == kHashResult) {
                    slots[i] = *(u32*)lbl_805FB89C;
                } else if (h == kHashNull) {
                    slots[i] = 0;
                    isNull = 1;
                } else {
                    u32 key = h;
                    slots[i] = PtrSlot(fn_801B7198(&key, 0));
                }
                if (i == 0 && slots[i] == 0 && isNull == 0) {
                    ok = 0;
                    break;
                }
            }
        }
    }

    if (ok != 0 && (arg2 & 0xFF00)) {
        for (u32 i = 0; i < arg1; i++) {
            if (arg2 & (0x100u << i)) {
                u32 ref = slots[i];
                fn_801EE078((u8*)&ref, arg3, arg4);
                if (fn_801ED368((u8*)&ref) != 0) {
                    slots[i] = ref;
                } else {
                    ok = 0;
                    slots[i] = 0;
                }
            }
        }
    }

    if (ok != 0 && (arg2 & 0xFF0000)) {
        for (u32 i = 0; i < arg1; i++) {
            if (arg2 & (0x10000u << i)) {
                u32 h = slots[i];
                u8* obj;
                if (h == kHashThis) {
                    obj = (u8*)arg3;
                } else if (h == kHashOther) {
                    obj = (u8*)arg4;
                } else if (h == kHashResult) {
                    obj = *(u8**)lbl_805FB89C;
                } else {
                    u32 key = h;
                    obj = (u8*)fn_801B7198(&key, 0);
                }
                s32* p;
                if (obj != NULL && (p = *(s32**)(obj + 0xA4)) != NULL && *p != 0) {
                    slots[i] = (u32)*p;
                } else {
                    slots[i] = 0;
                }
                if (slots[i] == 0) {
                    ok = 0;
                    break;
                }
            }
        }
    }

    if (ok != 0 && (arg2 & 0xFF000000)) {
        for (u32 i = 0; i < arg1; i++) {
            if ((arg2 & (0x01000000u << i)) && slots[i] == kHashResult) {
                slots[i] = *(u32*)lbl_805FB89C;
            }
        }
    }
    return ok;
}

// 0x801E28E0
// Runs one update pass over the entries of the list at +4, bracketed by fn_8010AB0C(0,0) and
// fn_8010AB0C(1,1). If bit 1 of the owner's flags is set the entries are processed with
// fn_801E32E4 after the camera reset, otherwise with fn_801E2BB4 before it.
void fn_801E28E0(u8* arg0) {
    if (*(u8**)(arg0 + 0x4) != NULL) {
        fn_8010AB0C(0, 0);
        if (fn_801E29C8(arg0) != 0) {
            fn_800446BC(lbl_8047C640, 0);
            u8* e = (u8*)(uintptr_t)fn_801CB96C(*(u8**)(arg0 + 0x4));
            while (e != NULL) {
                fn_801E32E4(e);
                e = (u8*)(uintptr_t)fn_801CBCDC(*(u8**)(arg0 + 0x4), e);
            }
        } else {
            u8* e = (u8*)(uintptr_t)fn_801CB96C(*(u8**)(arg0 + 0x4));
            while (e != NULL) {
                fn_801E2BB4(e);
                e = (u8*)(uintptr_t)fn_801CBCDC(*(u8**)(arg0 + 0x4), e);
            }
            fn_800446BC(lbl_8047C640, 0);
        }
        fn_8010AB0C(1, 1);
    }
}

}  // extern "C"
